
/*
 *  AudioStream.cpp
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
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
}

#include <cstring>
#include <iostream>
#include "AudioStream.hpp"
#include "Log.hpp"
#include <sfeMovie/Movie.hpp>

namespace sfe
{
    namespace
    {
        void waitForStatusUpdate(const sf::SoundStream& stream, sf::SoundStream::Status expectedStatus)
        {
            // Wait for status to update
            sf::Clock timeout;
            while (stream.getStatus() != expectedStatus && timeout.getElapsedTime() < sf::seconds(5))
                sf::sleep(sf::microseconds(10));
            CHECK(timeout.getElapsedTime() < sf::seconds(5), "Audio did not reach state " + s(expectedStatus) + " within 5 seconds");
        }
    }

    AudioStream::AudioStream(AVFormatContext*& formatCtx, AVStream*& stream, DataSource& dataSource,
                             std::shared_ptr<Timer> timer) :
    Stream(formatCtx, stream, dataSource, timer),
    
    // Public properties
    m_sampleRate(0),
    
    // Private data
    m_samplesBuffer(nullptr),
    m_audioFrame(nullptr),
    
    // Resampling
    m_swrCtx(nullptr),
    m_dstNbSamples(0),
    m_maxDstNbSamples(0),
    m_dstNbChannels(0),
    m_dstLinesize(0),
    m_dstData(nullptr)
    {
        m_audioFrame = av_frame_alloc();
        CHECK(m_audioFrame, "AudioStream::AudioStream() - out of memory");
        
        // Get some audio informations
        m_sampleRate = m_stream->codec->sample_rate;
        
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
    AudioStream::~AudioStream()
    {
        if (m_audioFrame)
        {
            av_frame_free(&m_audioFrame);
        }
        
        if (m_samplesBuffer)
        {
            av_free(m_samplesBuffer);
        }
        
        if (m_dstData)
        {
            av_freep(&m_dstData[0]);
        }
        av_freep(&m_dstData);
        
        swr_free(&m_swrCtx);
    }
    
    void AudioStream::flushBuffers()
    {
        sf::SoundStream::Status sfStatus = sf::SoundStream::getStatus();
        CHECK (sfStatus != sf::SoundStream::Playing, "Trying to flush while audio is playing, this will introduce an audio glitch!");
        
        // Flush audio driver/OpenAL/SFML buffer
        if (sfStatus != sf::SoundStream::Stopped)
            sf::SoundStream::stop();
        
        Stream::flushBuffers();
    }
    
    MediaType AudioStream::getStreamKind() const
    {
        return Audio;
    }
    
    void AudioStream::update()
    {
        sf::SoundStream::Status sfStatus = sf::SoundStream::getStatus();
        
        switch (sfStatus)
        {
            case sf::SoundStream::Playing:
                setStatus(sfe::Playing);
                break;
                
            case sf::SoundStream::Paused:
                setStatus(sfe::Paused);
                break;
                
            case sf::SoundStream::Stopped:
                setStatus(sfe::Stopped);
                break;
                
            default:
                break;
        }
    }
    
    bool AudioStream::fastForward(sf::Time targetPosition)
    {
        sf::Time currentPosition;
        sf::Time pktDuration;
        
        do
        {
            currentPosition = computePosition();
            AVPacket* packet = popEncodedData();
            
            if (! packet)
            {
                sfeLogError("Fast-forwarding failure in audio stream, " +
                            "did reach end of stream (target position=" +
                            s(targetPosition.asSeconds()) + "s)");
                return false;
            }
            
            pktDuration = packetDuration(packet);
            
            if (currentPosition + pktDuration > targetPosition)
            {
                // Reinsert, we don't want to decode now
                prependEncodedData(packet);
            }
            else
            {
                av_free_packet(packet);
                av_free(packet);
            }
        }
        while (currentPosition + pktDuration <= targetPosition);
        
        return true;
    }
    
    bool AudioStream::onGetData(sf::SoundStream::Chunk& data)
    {
        AVPacket* packet = nullptr;
        data.samples = m_samplesBuffer;
        
        while (data.sampleCount < av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO) * m_sampleRate &&
               (nullptr != (packet = popEncodedData())))
        {
            bool needsMoreDecoding = false;
            bool gotFrame = false;
            
            do
            {
                needsMoreDecoding = decodePacket(packet, m_audioFrame, gotFrame);
                
                if (gotFrame)
                {
                    uint8_t* samples = nullptr;
                    int nbSamples = 0;
                    int samplesLength = 0;
                    
                    resampleFrame(m_audioFrame, samples, nbSamples, samplesLength);
                    CHECK(samples, "AudioStream::onGetData() - resampleFrame() error");
                    CHECK(nbSamples > 0, "AudioStream::onGetData() - resampleFrame() error");
                    CHECK(nbSamples == samplesLength / 2, "AudioStream::onGetData() resampleFrame() inconsistency");
                    
                    CHECK(data.sampleCount + nbSamples < m_sampleRate * 2 * av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO), "AudioStream::onGetData() - Going to overflow!!");
                    
                    std::memcpy((void *)(data.samples + data.sampleCount),
                                samples, samplesLength);
                    data.sampleCount += nbSamples;
                }
            } while (needsMoreDecoding);
            
            av_free_packet(packet);
            av_free(packet);
        }
        
        if (!packet)
        {
            sfeLogDebug("No more audio packets, do not go further");
        }
        return (packet != nullptr);
    }
    
    void AudioStream::onSeek(sf::Time timeOffset)
    {
        //        CHECK(0, "AudioStream::onSeek() - not implemented");
    }
    
    bool AudioStream::decodePacket(AVPacket* packet, AVFrame* outputFrame, bool& gotFrame)
    {
        bool needsMoreDecoding = false;
        int igotFrame = 0;
        
        int decodedLength = avcodec_decode_audio4(m_stream->codec, outputFrame, &igotFrame, packet);
        gotFrame = (igotFrame != 0);
        CHECK(decodedLength >= 0, "AudioStream::decodePacket() - error: decodedLength=" + s(decodedLength));
        
        if (decodedLength < packet->size)
        {
            needsMoreDecoding = true;
            packet->data += decodedLength;
            packet->size -= decodedLength;
        }
        
        return needsMoreDecoding;
    }
    
    void AudioStream::initResampler()
    {
        CHECK0(m_swrCtx, "AudioStream::initResampler() - resampler already initialized");
        int err = 0;
        
        /* create resampler context */
        m_swrCtx = swr_alloc();
        CHECK(m_swrCtx, "AudioStream::initResampler() - out of memory");
        
        // Some media files don't define the channel layout, in this case take a default one
        // according to the channels' count
        if (m_stream->codec->channel_layout == 0)
        {
            m_stream->codec->channel_layout = av_get_default_channel_layout(m_stream->codec->channels);
        }
        
        /* set options */
        av_opt_set_int        (m_swrCtx, "in_channel_layout",  m_stream->codec->channel_layout, 0);
        av_opt_set_int        (m_swrCtx, "in_sample_rate",     m_stream->codec->sample_rate,    0);
        av_opt_set_sample_fmt (m_swrCtx, "in_sample_fmt",      m_stream->codec->sample_fmt,     0);
        av_opt_set_int        (m_swrCtx, "out_channel_layout", AV_CH_LAYOUT_STEREO,             0);
        av_opt_set_int        (m_swrCtx, "out_sample_rate",    m_stream->codec->sample_rate,    0);
        av_opt_set_sample_fmt (m_swrCtx, "out_sample_fmt",     AV_SAMPLE_FMT_S16,               0);
        
        /* initialize the resampling context */
        err = swr_init(m_swrCtx);
        CHECK(err >= 0, "AudioStream::initResampler() - resampling context initialization error");
        
        /* compute the number of converted samples: buffering is avoided
         * ensuring that the output buffer will contain at least all the
         * converted input samples */
        m_maxDstNbSamples = m_dstNbSamples = 1024;
        
        /* Create the resampling output buffer */
        m_dstNbChannels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
        err = av_samples_alloc_array_and_samples(&m_dstData, &m_dstLinesize, m_dstNbChannels,
                                                 m_dstNbSamples, AV_SAMPLE_FMT_S16, 0);
        CHECK(err >= 0, "AudioStream::initResampler() - av_samples_alloc_array_and_samples error");
    }
    
    void AudioStream::resampleFrame(const AVFrame* frame, uint8_t*& outSamples, int& outNbSamples, int& outSamplesLength)
    {
        CHECK(m_swrCtx, "AudioStream::resampleFrame() - resampler is not initialized, call AudioStream::initResamplerFirst() !");
        CHECK(frame, "AudioStream::resampleFrame() - invalid argument");
        
        int src_rate, dst_rate, err, dst_bufsize;
        src_rate = dst_rate = frame->sample_rate;
        
        /* compute destination number of samples */
        m_dstNbSamples = av_rescale_rnd(swr_get_delay(m_swrCtx, src_rate) +
                                        frame->nb_samples, dst_rate, src_rate, AV_ROUND_UP);
        if (m_dstNbSamples > m_maxDstNbSamples)
        {
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
    
    sf::Time AudioStream::packetDuration(const AVPacket* packet) const
    {
        CHECK(packet, "inconcistency error: null packet");
        CHECK(packet->stream_index == m_streamID, "Asking for audio duration of a packet for a different stream!");
        CHECK(packet->duration != 0, "packet duration is unknown");
        
        return sf::seconds(packet->duration * av_q2d(m_stream->time_base));
    }
    
    void AudioStream::willPlay(const Timer &timer)
    {
        Stream::willPlay(timer);
        
        if (Stream::getStatus() == sfe::Stopped)
        {
            sf::Time initialTime = sf::SoundStream::getPlayingOffset();
            sf::Clock timeout;
            sf::SoundStream::play();
            
            // Some audio drivers take time before the sound is actually played
            // To avoid desynchronization with the timer, we don't return
            // until the audio stream is actually started
            while (sf::SoundStream::getPlayingOffset() == initialTime && timeout.getElapsedTime() < sf::seconds(5))
                sf::sleep(sf::microseconds(10));
            
            CHECK(sf::SoundStream::getPlayingOffset() != initialTime, "is your audio device broken? Audio did not start within 5 seconds");
        }
        else
        {
            sf::SoundStream::play();
            waitForStatusUpdate(*this, sf::SoundStream::Playing);
        }
    }
    
    void AudioStream::didPlay(const Timer& timer, sfe::Status previousStatus)
    {
        CHECK(SoundStream::getStatus() == SoundStream::Playing, "AudioStream::didPlay() - willPlay() not executed!");
        Stream::didPlay(timer, previousStatus);
    }
    
    void AudioStream::didPause(const Timer& timer, sfe::Status previousStatus)
    {
        if (sf::SoundStream::getStatus() == sf::SoundStream::Playing)
        {
            sf::SoundStream::pause();
            waitForStatusUpdate(*this, sf::SoundStream::Paused);
        }
        
        Stream::didPause(timer, previousStatus);
    }
    
    void AudioStream::didStop(const Timer& timer, sfe::Status previousStatus)
    {
        sf::SoundStream::stop();
        waitForStatusUpdate(*this, sf::SoundStream::Stopped);
        
        Stream::didStop(timer, previousStatus);
    }
}
