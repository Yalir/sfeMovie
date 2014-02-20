
extern "C" {
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
	#include <libswscale/swscale.h>
	#include <libavutil/samplefmt.h>
}

#include <cstring>
#include "AudioStream.hpp"

namespace sfe {
	AudioStream::AudioStream(AVStreamRef stream, DataSource& dataSource, Timer& timer) :
	Stream(stream, dataSource, timer),
	m_channelsCount(0),
	m_sampleRate(0),
	m_samplesBuffer(NULL),
	m_audioFrame(NULL)
	{
		m_audioFrame = av_frame_alloc();
		CHECK(m_audioFrame, "AudioStream::AudioStream() - out of memory");
		
		// Get some audio informations
		m_channelsCount = m_codecCtx->channels;
		m_sampleRate = m_codecCtx->sample_rate;
		
		// Alloc a one second buffer
		m_samplesBuffer = (sf::Int16*)av_malloc(sizeof(sf::Int16) * m_channelsCount * m_sampleRate);
		CHECK(m_samplesBuffer, "AudioStream::AudioStream() - out of memory");
		
		// Initialize the sf::SoundStream
		sf::SoundStream::initialize(m_channelsCount, m_sampleRate);
	}
	
	/** Default destructor
	 */
	AudioStream::~AudioStream(void)
	{
		if (m_audioFrame) {
			av_frame_free(&m_audioFrame);
		}
		
		if (m_samplesBuffer) {
			av_free(m_samplesBuffer);
		}
	}
	
	/* A/V control */
	
	/** Start playing this stream
	 */
	void AudioStream::play(void)
	{
		
	}
	
	/** Pause stream playback
	 */
	void AudioStream::pause(void)
	{
		
	}
	
	/** Stop stream playback and go back to beginning
	 */
	void AudioStream::stop(void)
	{
		
	}
	
	AudioStream::Kind AudioStream::getStreamKind(void) const
	{
		return AUDIO_STREAM;
	}
	
	bool AudioStream::onGetData(sf::SoundStream::Chunk& data)
	{
		AVPacketRef packet;
		data.samples = m_samplesBuffer;
		
		while (data.sampleCount < m_channelsCount * m_sampleRate &&
			   (NULL != (packet = popEncodedData()))) {
			
			while (decodePacket(packet, m_audioFrame)) {
				size_t unpadded_linesize = m_audioFrame->nb_samples *
					av_get_bytes_per_sample((AVSampleFormat)m_audioFrame->format);
				CHECK(av_get_bytes_per_sample((AVSampleFormat)m_audioFrame->format) == 2, "AudioStream::onGetData() - unsupported audio sample format");
				
				std::memcpy((void *)(data.samples + data.sampleCount),
							m_audioFrame->extended_data[0], unpadded_linesize);
				data.sampleCount += m_audioFrame->nb_samples;
			}
			
			av_free_packet(packet);
			av_free(packet);
		}
		
		return false;
	}
	
	void AudioStream::onSeek(sf::Time timeOffset)
	{
		CHECK(0, "AudioStream::onSeek() - not implemented");
	}
	
	bool AudioStream::decodePacket(AVPacketRef packet, AVFrameRef outputFrame)
	{
		bool needsMoreDecoding = false;
		int gotFrame = 0;
		int decodedLength = avcodec_decode_audio4(m_codecCtx, outputFrame, &gotFrame, packet);
		CHECK(decodedLength >= 0, "AudioStream::decodePacket() - error: " + std::string(av_err2str(decodedLength)));
		
		if (decodedLength < packet->size) {
			needsMoreDecoding = true;
		}
		
		return needsMoreDecoding;
	}
	
	void AudioStream::didPlay(const Timer& timer, Timer::Status previousStatus)
	{
		sf::SoundStream::play();
	}
	
	void AudioStream::didPause(const Timer& timer, Timer::Status previousStatus)
	{
	}
	
	void AudioStream::didStop(const Timer& timer, Timer::Status previousStatus)
	{
	}
}
