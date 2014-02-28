
extern "C" {
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
	#include <libavutil/samplefmt.h>
	#include <libavutil/opt.h>
	#include <libavutil/channel_layout.h>
	#include <libswresample/swresample.h>
}

#include <cstring>
#include <iostream>
#include "AudioStream.hpp"

namespace sfe {
	AudioStream::AudioStream(AVStreamRef stream, DataSource& dataSource, Timer& timer) :
	Stream(stream, dataSource, timer),
	
	// Public properties
	m_channelsCount(0),
	m_sampleRate(0),
	
	// Private data
	m_samplesBuffer(NULL),
	m_audioFrame(NULL),
	
	// Resampling
	m_swrCtx(NULL),
	m_srcNbSamples(1024),
	m_dstNbSamples(0),
	m_maxDstNbSamples(0),
	m_srcNbChannels(0),
	m_dstNbChannels(0),
	m_srcLinesize(0),
	m_dstLinesize(0),
	m_srcData(NULL),
	m_dstData(NULL)
	{
		m_audioFrame = av_frame_alloc();
		CHECK(m_audioFrame, "AudioStream::AudioStream() - out of memory");
		
		// Get some audio informations
		m_channelsCount = m_codecCtx->channels;
		m_sampleRate = m_codecCtx->sample_rate;
		
		// Alloc a two seconds buffer
		m_samplesBuffer = (sf::Int16*)av_malloc(sizeof(sf::Int16) * av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO) * m_sampleRate * 2);
		CHECK(m_samplesBuffer, "AudioStream::AudioStream() - out of memory");
		
		// Initialize the sf::SoundStream
		// Whatever the channel count is, it'll we resampled to stereo
		sf::SoundStream::initialize(av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO), m_sampleRate);
		
		// Initialize resampler to be able to give signed 16 bits samples to SFML
		initResampler();
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
	
	MediaType AudioStream::getStreamKind(void) const
	{
		return MEDIA_TYPE_AUDIO;
	}
	
	bool AudioStream::onGetData(sf::SoundStream::Chunk& data)
	{
		AVPacketRef packet;
		data.samples = m_samplesBuffer;
		
		while (data.sampleCount < m_channelsCount * m_sampleRate &&
			   (NULL != (packet = popEncodedData()))) {
			
			bool needsMoreDecoding = false;
			bool gotFrame = false;
			
			do {
				needsMoreDecoding = decodePacket(packet, m_audioFrame, gotFrame);
				
				if (gotFrame) {
					uint8_t* samples = NULL;
					int nbSamples = 0;
					int samplesLength = 0;
					
					resampleFrame(m_audioFrame, samples, nbSamples, samplesLength);
					CHECK(samples, "AudioStream::onGetData() - resampleFrame() error");
					CHECK(nbSamples > 0, "AudioStream::onGetData() - resampleFrame() error");
					CHECK(nbSamples == samplesLength / 2, "AudioStream::onGetData() resampleFrame() inconsistency");
					
					std::memcpy((void *)(data.samples + data.sampleCount),
								samples, samplesLength);
					data.sampleCount += nbSamples;
				}
			} while (needsMoreDecoding);
			
			av_free_packet(packet);
			av_free(packet);
		}
		
		return (data.sampleCount >= m_channelsCount * m_sampleRate);
	}
	
	void AudioStream::onSeek(sf::Time timeOffset)
	{
//		CHECK(0, "AudioStream::onSeek() - not implemented");
	}
	
	bool AudioStream::decodePacket(AVPacketRef packet, AVFrameRef outputFrame, bool& gotFrame)
	{
		bool needsMoreDecoding = false;
		int igotFrame = 0;
		
		int decodedLength = avcodec_decode_audio4(m_codecCtx, outputFrame, &igotFrame, packet);
		gotFrame = (igotFrame != 0);
		CHECK(decodedLength >= 0, "AudioStream::decodePacket() - error: " + std::string(av_err2str(decodedLength)));
		
		if (decodedLength < packet->size) {
			needsMoreDecoding = true;
		}
		
		return needsMoreDecoding;
	}
	
	void AudioStream::initResampler(void)
	{
		int err = 0;
		
		/* create resampler context */
		m_swrCtx = swr_alloc();
		CHECK(m_swrCtx, "AudioStream::initResampler() - out of memory");
		
		// Some media files don't define the channel layout, in this case take a default one
		// according to the channels' count
		if (m_codecCtx->channel_layout == 0) {
			m_codecCtx->channel_layout = av_get_default_channel_layout(m_channelsCount);
		}
		
		/* set options */
		av_opt_set_int(m_swrCtx, "in_channel_layout",    m_codecCtx->channel_layout, 0);
		av_opt_set_int(m_swrCtx, "in_sample_rate",       m_codecCtx->sample_rate, 0);
		av_opt_set_sample_fmt(m_swrCtx, "in_sample_fmt", m_codecCtx->sample_fmt, 0);
		av_opt_set_int(m_swrCtx, "out_channel_layout",    AV_CH_LAYOUT_STEREO, 0);
		av_opt_set_int(m_swrCtx, "out_sample_rate",       m_codecCtx->sample_rate, 0);
		av_opt_set_sample_fmt(m_swrCtx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
		
		/* initialize the resampling context */
		err = swr_init(m_swrCtx);
		CHECK(err >= 0, "AudioStream::initResampler() - resampling context initialization error");
		
		/* allocate source and destination samples buffers */
		m_srcNbChannels = av_get_channel_layout_nb_channels(m_codecCtx->channel_layout);
		err = av_samples_alloc_array_and_samples(&m_srcData, &m_srcLinesize, m_srcNbChannels,
												 m_codecCtx->sample_rate, m_codecCtx->sample_fmt, 0);
		CHECK(err >= 0, "AudioStream::initResampler() - av_samples_alloc_array_and_samples error");
		
		/* compute the number of converted samples: buffering is avoided
		 * ensuring that the output buffer will contain at least all the
		 * converted input samples */
		m_maxDstNbSamples = m_dstNbSamples =
        av_rescale_rnd(m_srcNbSamples, m_codecCtx->sample_rate, m_codecCtx->sample_rate, AV_ROUND_UP);

		/* Create the resampling output buffer */
		m_dstNbChannels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
		err = av_samples_alloc_array_and_samples(&m_dstData, &m_dstLinesize, m_dstNbChannels,
												 m_dstNbSamples, AV_SAMPLE_FMT_S16, 0);
		CHECK(err >= 0, "AudioStream::initResampler() - av_samples_alloc_array_and_samples error");
	}
	
	void AudioStream::resampleFrame(const AVFrameRef frame, uint8_t*& outSamples, int& outNbSamples, int& outSamplesLength)
	{
		CHECK(m_swrCtx, "AudioStream::resampleFrame() - resampler is not initialized, call AudioStream::initResamplerFirst() !");
		CHECK(frame, "AudioStream::resampleFrame() - invalid argument");
		
		int src_rate, dst_rate, err, dst_bufsize;
		src_rate = dst_rate = frame->sample_rate;
		
		/* compute destination number of samples */
        m_dstNbSamples = av_rescale_rnd(swr_get_delay(m_swrCtx, src_rate) +
                                        frame->nb_samples, dst_rate, src_rate, AV_ROUND_UP);
        if (m_dstNbSamples > m_maxDstNbSamples) {
            av_free(m_dstData[0]);
            err = av_samples_alloc(m_dstData, &m_dstLinesize, m_dstNbChannels,
                                   m_dstNbSamples, AV_SAMPLE_FMT_S16, 1);
            CHECK(err >= 0, "AudioStream::resampleFrame() - out of memory");
            m_maxDstNbSamples = m_dstNbSamples;
        }
		
        /* convert to destination format */
        err = swr_convert(m_swrCtx, m_dstData, m_dstNbSamples, (const uint8_t **)frame->extended_data, frame->nb_samples);
		CHECK(err >= 0, "AudioStream::resampleFrame() - swr_convert() error");
        
        dst_bufsize = av_samples_get_buffer_size(&m_dstLinesize, m_dstNbChannels,
                                                 err, AV_SAMPLE_FMT_S16, 1);
        CHECK(dst_bufsize >= 0, "AudioStream::resampleFrame() - av_samples_get_buffer_size() error");
		
		outNbSamples = dst_bufsize / av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
		outSamplesLength = dst_bufsize;
		outSamples = m_dstData[0];
	}
	
	void AudioStream::didPlay(const Timer& timer, Timer::Status previousStatus)
	{
		sf::SoundStream::play();
	}
	
	void AudioStream::didPause(const Timer& timer, Timer::Status previousStatus)
	{
		sf::SoundStream::pause();
	}
	
	void AudioStream::didStop(const Timer& timer, Timer::Status previousStatus)
	{
		sf::SoundStream::stop();
	}
}
