
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
#include <iostream>
#include <stdexcept>

namespace sfe {
	Demuxer::Demuxer(const std::string& sourceFile) :
	m_avFormatCtx(NULL),
	m_streams()
	{
		CHECK(sourceFile.size(), "Demuxer() - invalid argument: sourceFile");
		
		int err = 0;
		
		// Load all the decoders
		ONCE(av_register_all());
		ONCE(avcodec_register_all());
		
		// Open the movie file
		err = avformat_open_input(&m_avFormatCtx, sourceFile.c_str(), NULL, NULL);
		CHECK0(err, "Demuxer() - error while opening media");
		CHECK(m_avFormatCtx, "Demuxer() - inconsistency: media context cannot be null");
		
		// Read the general movie informations
		err = avformat_find_stream_info(m_avFormatCtx, NULL);
		CHECK0(err, "Demuxer() - error while retreiving media information");
		
		// Find all interesting streams
		for (int i = 0; i < m_avFormatCtx->nb_streams; i++) {
			AVStreamRef ffstream = m_avFormatCtx->streams[i];
			
			try {
				switch (ffstream->codec->codec_type) {
					case AVMEDIA_TYPE_VIDEO:
						m_streams[ffstream->index] = new VideoStream(ffstream, *this);
						break;
						
					case AVMEDIA_TYPE_AUDIO:
						m_streams[ffstream->index] = new AudioStream(ffstream, *this);
						break;
						
						/** TODO
						 case AVMEDIA_TYPE_SUBTITLE:
						 m_streams.push_back(new SubtitleStream(ffstream));
						 break;
						 */
						
					default:
						std::cerr << "Demuxer() - stream '" << av_get_media_type_string(ffstream->codec->codec_type) << "' ignored" << std::endl;
						break;
				}
			} catch (std::runtime_error& e) {
				std::cerr << "Demuxer() - " << e.what() << std::endl;
			}
		}
	}
	
	Demuxer::~Demuxer(void)
	{
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
	
	void Demuxer::feedStream(Stream& stream)
	{
		while (!didReachEndOfFile() && stream.needsMoreData()) {
			AVPacketRef pkt = readPacket();
			
			if (!pkt) {
				m_eofReached = true;
			} else {
				if (!distributePacket(pkt)) {
					std::cerr << "Demuxer::feedStreams() - packet with stream index "
					<< pkt->stream_index << " not handled and dropped" << std::endl;
					av_free_packet(pkt);
					av_free(pkt);
				}
			}
		}
	}
	
	bool Demuxer::didReachEndOfFile(void) const
	{
		return m_eofReached;
	}
	
	AVPacketRef Demuxer::readPacket(void)
	{
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
		CHECK(packet, "Demuxer::distributePacket() - invalid argument");
		
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
		feedStream(starvingStream);
	}
}
