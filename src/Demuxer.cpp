
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
	std::set<std::pair<std::string, MediaType> > Demuxer::g_availableDecoders;
	
	static void loadDecoders(void)
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
	
	const std::set<std::pair<std::string, MediaType> >& Demuxer::getAvailableDecoders(void)
	{
		AVCodecRef codec = NULL;
		loadDecoders();
		
		if (g_availableDecoders.empty()) {
			while (NULL != (codec = av_codec_next(codec))) {
				MediaType type = AVMediaTypeToMediaType(codec->type);
				g_availableDecoders.insert(std::make_pair(avcodec_get_name(codec->id), type));
			}
		}
		
		return g_availableDecoders;
	}
	
	Demuxer::Demuxer(const std::string& sourceFile, Timer& timer) :
	m_avFormatCtx(NULL),
	m_eofReached(false),
	m_streams(),
	m_ignoredStreams()
	{
		CHECK(sourceFile.size(), "Demuxer::Demuxer() - invalid argument: sourceFile");
		
		int err = 0;
		
		// Load all the decoders
		loadDecoders();
		
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
//						std::cout << "Loaded " << avcodec_get_name(ffstream->codec->codec_id) << " video stream" << std::endl;
//						break;
						
					case AVMEDIA_TYPE_AUDIO:
						m_streams[ffstream->index] = new AudioStream(ffstream, *this, timer);
						std::cout << "Loaded " << avcodec_get_name(ffstream->codec->codec_id) << " audio stream" << std::endl;
						break;
						
						/** TODO
						 case AVMEDIA_TYPE_SUBTITLE:
						 m_streams.push_back(new SubtitleStream(ffstream));
						 break;
						 */
						
					default:
						m_ignoredStreams[ffstream->index] = std::string(std::string(av_get_media_type_string(ffstream->codec->codec_type)) + "/" + avcodec_get_name(ffstream->codec->codec_id));
						std::cerr << "Demuxer::Demuxer() - '" << m_ignoredStreams[ffstream->index] << "' stream ignored" << std::endl;
						break;
				}
			} catch (std::runtime_error& e) {
				std::cerr << "Demuxer::Demuxer() - " << e.what() << std::endl;
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
					std::cerr << "Demuxer::feedStream() - " << m_ignoredStreams[pkt->stream_index] << " packet not handled and dropped" << std::endl;
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
