
/*
 *  Stream.cpp
 *  sfeMovie project
 *
 *  Copyright (C) 2010-2014 Lucas Soltic
 *  lucas.soltic@orange.fr
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include "Demuxer.hpp"
#include "VideoStream.hpp"
#include "AudioStream.hpp"
#include "SubtitleStream.hpp"
#include "Threads.hpp"
#include "Log.hpp"
#include "Utilities.hpp"
#include <iostream>
#include <stdexcept>

namespace sfe {
	std::list<Demuxer::DemuxerInfo> Demuxer::g_availableDemuxers;
	std::list<Demuxer::DecoderInfo> Demuxer::g_availableDecoders;
	
	static void loadFFmpeg(void)
	{
		ONCE(av_register_all());
		ONCE(avcodec_register_all());
	}
	
	static MediaType AVMediaTypeToMediaType(AVMediaType type)
	{
		switch (type) {
			case AVMEDIA_TYPE_AUDIO:	return MEDIA_TYPE_AUDIO;
			case AVMEDIA_TYPE_SUBTITLE:	return MEDIA_TYPE_SUBTITLE;
			case AVMEDIA_TYPE_VIDEO:	return MEDIA_TYPE_VIDEO;
			default:					return MEDIA_TYPE_UNKNOWN;
		}
	}
	
	const std::list<Demuxer::DemuxerInfo>& Demuxer::getAvailableDemuxers(void)
	{
		AVInputFormat* demuxer = NULL;
		loadFFmpeg();
		
		if (g_availableDemuxers.empty()) {
			while (NULL != (demuxer = av_iformat_next(demuxer))) {
				DemuxerInfo info = {
					std::string(demuxer->name),
					std::string(demuxer->long_name)
				};
				
				g_availableDemuxers.push_back(info);
			}
		}
		
		return g_availableDemuxers;
	}
	
	const std::list<Demuxer::DecoderInfo>& Demuxer::getAvailableDecoders(void)
	{
		AVCodecRef codec = NULL;
		loadFFmpeg();
		
		if (g_availableDecoders.empty()) {
			while (NULL != (codec = av_codec_next(codec))) {
				DecoderInfo info = {
					avcodec_get_name(codec->id),
					codec->long_name,
					AVMediaTypeToMediaType(codec->type)
				};
				
				g_availableDecoders.push_back(info);
			}
		}
		
		return g_availableDecoders;
	}
	
	Demuxer::Demuxer(const std::string& sourceFile, Timer& timer) :
	m_avFormatCtx(NULL),
	m_eofReached(false),
	m_streams(),
	m_ignoredStreams(),
	m_synchronized(),
	m_timer(timer)
	{
		CHECK(sourceFile.size(), "Demuxer::Demuxer() - invalid argument: sourceFile");
		
		int err = 0;
		
		// Load all the decoders
		loadFFmpeg();
		
		// Open the movie file
		err = avformat_open_input(&m_avFormatCtx, sourceFile.c_str(), NULL, NULL);
		CHECK0(err, "Demuxer::Demuxer() - error while opening media: " + sourceFile);
		CHECK(m_avFormatCtx, "Demuxer() - inconsistency: media context cannot be null");
		
		// Read the general movie informations
		err = avformat_find_stream_info(m_avFormatCtx, NULL);
		CHECK0(err, "Demuxer::Demuxer() - error while retreiving media information");
		
		// Find all interesting streams
		for (int i = 0; i < m_avFormatCtx->nb_streams; i++) {
			AVStreamRef ffstream = m_avFormatCtx->streams[i];
			
			try {
				switch (ffstream->codec->codec_type) {
//					case AVMEDIA_TYPE_VIDEO:
//						m_streams[ffstream->index] = new VideoStream(ffstream, *this, timer);
//						sfeLogDebug("Loaded " + avcodec_get_name(ffstream->codec->codec_id) + " video stream");
//						break;
						
					case AVMEDIA_TYPE_AUDIO:
						m_streams[ffstream->index] = new AudioStream(ffstream, *this, timer);
						sfeLogDebug("Loaded " + avcodec_get_name(ffstream->codec->codec_id) + " audio stream");
						break;
						
						/** TODO
						 case AVMEDIA_TYPE_SUBTITLE:
						 m_streams.push_back(new SubtitleStream(ffstream));
						 break;
						 */
						
					default:
						m_ignoredStreams[ffstream->index] = std::string(std::string(av_get_media_type_string(ffstream->codec->codec_type)) + "/" + avcodec_get_name(ffstream->codec->codec_id));
						sfeLogWarning(m_ignoredStreams[ffstream->index] + "' stream ignored");
						break;
				}
			} catch (std::runtime_error& e) {
				std::cerr << "Demuxer::Demuxer() - " << e.what() << std::endl;
			}
		}
	}
	
	Demuxer::~Demuxer(void)
	{
		if (m_timer.getStatus() != Timer::Stopped)
			m_timer.stop();
		
		while (m_streams.size()) {
			delete m_streams.begin()->second;
			m_streams.erase(m_streams.begin());
		}
		
		if (m_avFormatCtx) {
			avformat_close_input(&m_avFormatCtx);
		}
	}
	
	const std::map<int, Stream*>& Demuxer::getStreams(void) const
	{
		return m_streams;
	}
	
	
	std::set<Stream*> Demuxer::getStreamsOfType(MediaType type) const
	{
		std::set<Stream*> streamSet;
		
		std::map<int, Stream*>::const_iterator it;
		
		for (it = m_streams.begin(); it != m_streams.end(); it++) {
			if (it->second->getStreamKind() == type)
				streamSet.insert(it->second);
		}
		
		return streamSet;
	}
	
	void Demuxer::feedStream(Stream& stream)
	{
		sf::Lock l(m_synchronized);
		
//		sfeLogDebug(Threads::currentThreadName());
		
		while (!didReachEndOfFile() && stream.needsMoreData()) {
			AVPacketRef pkt = readPacket();
			
			if (!pkt) {
				m_eofReached = true;
			} else {
				if (!distributePacket(pkt)) {
					sfeLogWarning(m_ignoredStreams[pkt->stream_index] + " packet not handled and dropped");
					av_free_packet(pkt);
					av_free(pkt);
				}
			}
		}
	}
	
	void Demuxer::updateVideoStreams(void)
	{
		std::set<Stream*> streams = getStreamsOfType(MEDIA_TYPE_VIDEO);
		std::set<Stream*>::iterator it;
		
		for (it = streams.begin();it != streams.end(); it++) {
			VideoStream* vStream = dynamic_cast<VideoStream*>(*it);
			CHECK(vStream, "Demuxer::updateVideoStreams() - got non video streams");
			
			vStream->updateTexture();
		}
	}
	
	bool Demuxer::didReachEndOfFile(void) const
	{
		return m_eofReached;
	}
	
	AVPacketRef Demuxer::readPacket(void)
	{
		sf::Lock l(m_synchronized);
//		sfeLogDebug(Threads::currentThreadName());
		
		AVPacket *pkt = NULL;
		int err = 0;
		
		pkt = (AVPacket *)av_malloc(sizeof(*pkt));
		CHECK(pkt, "Demuxer::readPacket() - out of memory");
		av_init_packet(pkt);
		
		err = av_read_frame(m_avFormatCtx, pkt);
		
		if (err < 0) {
			av_free_packet(pkt);
			av_free(pkt);
			pkt = NULL;
		}
		
		return pkt;
	}
	
	bool Demuxer::distributePacket(AVPacketRef packet)
	{
		sf::Lock l(m_synchronized);
		CHECK(packet, "Demuxer::distributePacket() - invalid argument");
//		sfeLogDebug(Threads::currentThreadName());
		
		bool result = false;
		std::map<int, Stream*>::iterator it = m_streams.find(packet->stream_index);
		
		if (it != m_streams.end()) {
			it->second->pushEncodedData(packet);
			result = true;
		}
		
		return result;
	}
	
	void Demuxer::requestMoreData(Stream& starvingStream)
	{
		sf::Lock l(m_synchronized);
//		sfeLogDebug(Threads::currentThreadName());
		
		feedStream(starvingStream);
	}
}
