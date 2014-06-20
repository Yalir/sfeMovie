
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
#include <stdint.h>
}

#include "Demuxer.hpp"
#include "VideoStream.hpp"
#include "AudioStream.hpp"
#include "SubtitleStream.hpp"
#include "Log.hpp"
#include "Utilities.hpp"
#include <iostream>
#include <stdexcept>

namespace sfe {
	std::list<Demuxer::DemuxerInfo> Demuxer::g_availableDemuxers;
	std::list<Demuxer::DecoderInfo> Demuxer::g_availableDecoders;
	
	static void loadFFmpeg()
	{
		ONCE(av_register_all());
		ONCE(avcodec_register_all());
		ONCE(Log::initialize());
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
	
	const std::list<Demuxer::DemuxerInfo>& Demuxer::getAvailableDemuxers()
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
	
	const std::list<Demuxer::DecoderInfo>& Demuxer::getAvailableDecoders()
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
	
	Demuxer::Demuxer(const std::string& sourceFile, Timer& timer, VideoStream::Delegate& videoDelegate) :
	m_formatCtx(NULL),
	m_eofReached(false),
	m_streams(),
	m_ignoredStreams(),
	m_synchronized(),
	m_timer(timer),
	m_connectedAudioStream(NULL),
	m_connectedVideoStream(NULL),
	m_duration(sf::Time::Zero)
	{
		CHECK(sourceFile.size(), "Demuxer::Demuxer() - invalid argument: sourceFile");
		
		int err = 0;
		
		// Load all the decoders
		loadFFmpeg();
		
		// Open the movie file
		err = avformat_open_input(&m_formatCtx, sourceFile.c_str(), NULL, NULL);
		CHECK0(err, "Demuxer::Demuxer() - error while opening media: " + sourceFile);
		CHECK(m_formatCtx, "Demuxer() - inconsistency: media context cannot be null");
		
		// Read the general movie informations
		err = avformat_find_stream_info(m_formatCtx, NULL);
		CHECK0(err, "Demuxer::Demuxer() - error while retreiving media information");
		
		// Get the media duration if possible (otherwise rely on the streams)
		if (m_formatCtx->duration != AV_NOPTS_VALUE)
		{
            long secs, us;
            secs = m_formatCtx->duration / AV_TIME_BASE;
            us = m_formatCtx->duration % AV_TIME_BASE;
			m_duration = sf::seconds(secs + (float)us / AV_TIME_BASE);
		}
		
		// Find all interesting streams
		for (int i = 0; i < m_formatCtx->nb_streams; i++) {
			AVStreamRef ffstream = m_formatCtx->streams[i];
			
			try {
				switch (ffstream->codec->codec_type) {
					case AVMEDIA_TYPE_VIDEO:
						m_streams[ffstream->index] = new VideoStream(m_formatCtx, ffstream, *this, timer, videoDelegate);
						
						if (m_duration == sf::Time::Zero) {
							extractDurationFromStream(ffstream);
						}
						
						sfeLogDebug("Loaded " + avcodec_get_name(ffstream->codec->codec_id) + " video stream");
						break;
						
					case AVMEDIA_TYPE_AUDIO:
						m_streams[ffstream->index] = new AudioStream(m_formatCtx, ffstream, *this, timer);
						
						if (m_duration == sf::Time::Zero) {
							extractDurationFromStream(ffstream);
						}
						
						sfeLogDebug("Loaded " + avcodec_get_name(ffstream->codec->codec_id) + " audio stream");
						break;
						
						/** TODO
						 case AVMEDIA_TYPE_SUBTITLE:
						 m_streams.push_back(new SubtitleStream(ffstream));
						 break;
						 */
						
					default:
						m_ignoredStreams[ffstream->index] = std::string(std::string(av_get_media_type_string(ffstream->codec->codec_type)) + "/" + avcodec_get_name(ffstream->codec->codec_id));
						sfeLogDebug(m_ignoredStreams[ffstream->index] + "' stream ignored");
						break;
				}
			} catch (std::runtime_error& e) {
				std::cerr << "Demuxer::Demuxer() - " << e.what() << std::endl;
			}
		}
		
		if (m_duration == sf::Time::Zero) {
			sfeLogWarning("The media duration could not be retreived");
		}
		
		m_timer.addObserver(*this);
	}
	
	Demuxer::~Demuxer()
	{
		if (m_timer.getStatus() != Stopped)
			m_timer.stop();
		
		m_timer.removeObserver(*this);
		
		while (m_streams.size()) {
			delete m_streams.begin()->second;
			m_streams.erase(m_streams.begin());
		}
		
		if (m_formatCtx) {
			avformat_close_input(&m_formatCtx);
		}
	}
	
	const std::map<int, Stream*>& Demuxer::getStreams() const
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
	
	std::vector<StreamEntry> Demuxer::computeStreamEntries() const
	{
		std::vector<StreamEntry> entries;
		std::set<Stream*> streamSet;
		std::map<int, Stream*>::const_iterator it;
		
		for (it = m_streams.begin(); it != m_streams.end(); it++) {
			StreamEntry entry;
			entry.index = it->first;
			entry.language = it->second->getLanguage();
			entry.type = it->second->getStreamKind();
			entries.push_back(entry);
		}
		
		return entries;
	}
	
	void Demuxer::selectAudioStream(AudioStream* stream)
	{
		Status oldStatus = m_timer.getStatus();
		
		if (oldStatus == Playing)
			m_timer.pause();
		
		if (stream != m_connectedAudioStream) {
			if (m_connectedAudioStream) {
				m_connectedAudioStream->disconnect();
			}
			
			if (stream)
				stream->connect();
			
			m_connectedAudioStream = stream;
		}
		
		if (oldStatus == Playing)
			m_timer.play();
	}
	
	void Demuxer::selectFirstAudioStream()
	{
		std::set<Stream*> audioStreams = getStreamsOfType(MEDIA_TYPE_AUDIO);
		if (audioStreams.size())
			selectAudioStream(dynamic_cast<AudioStream*>(*audioStreams.begin()));
	}
	
	AudioStream* Demuxer::getSelectedAudioStream() const
	{
		return dynamic_cast<AudioStream*>(m_connectedAudioStream);
	}
	
	void Demuxer::selectVideoStream(VideoStream* stream)
	{
		Status oldStatus = m_timer.getStatus();
		
		if (oldStatus == Playing)
			m_timer.pause();
		
		if (stream != m_connectedVideoStream) {
			if (m_connectedVideoStream) {
				m_connectedVideoStream->disconnect();
			}
			
			if (stream)
				stream->connect();
			
			m_connectedVideoStream = stream;
		}
		
		if (oldStatus == Playing)
			m_timer.play();
	}
	
	void Demuxer::selectFirstVideoStream()
	{
		std::set<Stream*> videoStreams = getStreamsOfType(MEDIA_TYPE_VIDEO);
		if (videoStreams.size())
			selectVideoStream(dynamic_cast<VideoStream*>(*videoStreams.begin()));
	}
	
	VideoStream* Demuxer::getSelectedVideoStream() const
	{
		return dynamic_cast<VideoStream*>(m_connectedVideoStream);
	}
	
	void Demuxer::feedStream(Stream& stream)
	{
		sf::Lock l(m_synchronized);
		sfeLogDebug("Feed " + MediaTypeToString(stream.getStreamKind()) + " stream");
		
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
	
	void Demuxer::update()
	{
		std::map<int, Stream*> streams = getStreams();
		std::map<int, Stream*>::iterator it;
		
//		std::cout << "Timer: " << m_timer.getOffset().asMilliseconds() << " ms" << std::endl;
		for (it = streams.begin();it != streams.end(); it++) {
			it->second->update();
		}
	}
	
	bool Demuxer::didReachEndOfFile() const
	{
		return m_eofReached;
	}
	
	sf::Time Demuxer::getDuration() const
	{
		return m_duration;
	}
	
	AVPacketRef Demuxer::readPacket()
	{
		sf::Lock l(m_synchronized);
		
		AVPacket *pkt = NULL;
		int err = 0;
		
		pkt = (AVPacket *)av_malloc(sizeof(*pkt));
		CHECK(pkt, "Demuxer::readPacket() - out of memory");
		av_init_packet(pkt);
		
		err = av_read_frame(m_formatCtx, pkt);
		
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
		
		bool result = false;
		std::map<int, Stream*>::iterator it = m_streams.find(packet->stream_index);
		
		if (it != m_streams.end()) {
			it->second->pushEncodedData(packet);
			result = true;
		}
		
		return result;
	}
	
	void Demuxer::extractDurationFromStream(AVStreamRef stream)
	{
		if (m_duration != sf::Time::Zero)
			return;
		
		if (stream->duration != AV_NOPTS_VALUE) {
            long secs, us;
            secs = stream->duration / AV_TIME_BASE;
            us = stream->duration % AV_TIME_BASE;
			m_duration = sf::seconds(secs + (float)us / AV_TIME_BASE);
		}
	}
	
	void Demuxer::requestMoreData(Stream& starvingStream)
	{
		sf::Lock l(m_synchronized);
		
		feedStream(starvingStream);
	}
	
	void Demuxer::resetEndOfFileStatus()
	{
		m_eofReached = false;
	}
	
	void Demuxer::willSeek(const Timer &timer, sf::Time position)
	{
		resetEndOfFileStatus();
		
		if (m_formatCtx->iformat->flags & AVFMT_SEEK_TO_PTS) {
			int64_t timestamp = 0;
			
			if (m_formatCtx->start_time != AV_NOPTS_VALUE)
				timestamp += m_formatCtx->start_time;
			
			int err = avformat_seek_file(m_formatCtx, -1, INT64_MIN, timestamp, INT64_MAX, AVSEEK_FLAG_BACKWARD);
			sfeLogDebug("Seek by PTS at timestamp=" + s(timestamp) + " returned " + s(err));
		} else {
			int err = avformat_seek_file(m_formatCtx, -1, INT64_MIN, 0, INT64_MAX, AVSEEK_FLAG_BACKWARD);
//			sfeLogDebug("Seek by PTS at timestamp=" + s(timestamp) + " returned " + s(err));
			
//			int err = av_seek_frame(m_formatCtx, m_streamID, -999999, AVSEEK_FLAG_BACKWARD);
			sfeLogDebug("Seek by DTS at timestamp " + s(-9999) + " returned " + s(err));
		}
	}
}
