
/*
 *  VideoStream.cpp
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
#include <libavutil/imgutils.h>
}

#include "VideoStream.hpp"
#include "Utilities.hpp"
#include "Log.hpp"

namespace sfe
{
    VideoStream::VideoStream(AVFormatContext*& formatCtx, AVStream*& stream,
                             DataSource& dataSource, std::shared_ptr<Timer> timer, Delegate& delegate) :
    Stream(formatCtx ,stream, dataSource, timer),
    m_texture(),
    m_rawVideoFrame(nullptr),
    m_rgbaVideoBuffer(),
    m_rgbaVideoLinesize(),
    m_delegate(delegate),
    m_swsCtx(nullptr),
    m_lastDecodedTimestamp(sf::Time::Zero)
    {
        int err;
        
        for (int i = 0; i < 4;i++)
        {
            m_rgbaVideoBuffer[i] = nullptr;
            m_rgbaVideoLinesize[i] = 0;
        }
        
        m_rawVideoFrame = av_frame_alloc();
        CHECK(m_rawVideoFrame, "VideoStream() - out of memory");
        
        // RGBA video buffer
        err = av_image_alloc(m_rgbaVideoBuffer, m_rgbaVideoLinesize,
                             m_stream->codec->width, m_stream->codec->height,
                             PIX_FMT_RGBA, 1);
        CHECK(err >= 0, "VideoStream() - av_image_alloc() error");
        
        // SFML video frame
        err = m_texture.create(m_stream->codec->width, m_stream->codec->height);
        CHECK(err, "VideoStream() - sf::Texture::create() error");
        
        initRescaler();
    }
    
    VideoStream::~VideoStream()
    {
        if (m_rawVideoFrame)
        {
            av_frame_free(&m_rawVideoFrame);
        }
        
        if (m_rgbaVideoBuffer[0])
        {
            av_freep(&m_rgbaVideoBuffer[0]);
        }
        
        if (m_swsCtx)
        {
            sws_freeContext(m_swsCtx);
        }
    }
    
    MediaType VideoStream::getStreamKind() const
    {
        return Video;
    }
    
    sf::Vector2i VideoStream::getFrameSize() const
    {
        return sf::Vector2i(m_stream->codec->width, m_stream->codec->height);
    }
    
    float VideoStream::getFrameRate() const
    {
        return static_cast<float>(av_q2d(av_guess_frame_rate(m_formatCtx, m_stream, nullptr)));
    }
    
    sf::Texture& VideoStream::getVideoTexture()
    {
        return m_texture;
    }
    
    void VideoStream::update()
    {
        while (getStatus() == Playing && getSynchronizationGap() < sf::Time::Zero)
        {
            if (!onGetData(m_texture))
            {
                setStatus(Stopped);
            }
            else
            {
                static const sf::Time skipFrameThreshold(sf::milliseconds(50));
                if (getSynchronizationGap() + skipFrameThreshold >= sf::Time::Zero)
                    m_delegate.didUpdateVideo(*this, m_texture);
            }
        }
    }
    
    void VideoStream::fastForward(sf::Time targetPosition)
    {
        while (computePosition() < targetPosition)
        {
            // We HAVE to decode the frames to get a full image when we reach the target position
            if (! onGetData(m_texture))
            {
                sfeLogError("Error while fast forwarding video stream up to position " +
                            s(targetPosition.asSeconds()) + "s");
                return;
            }
        }
    }
    
    bool VideoStream::onGetData(sf::Texture& texture)
    {
        AVPacket* packet = popEncodedData();
        bool gotFrame = false;
        bool goOn = false;
        
        if (packet)
        {
            goOn = true;
            
            while (!gotFrame && packet && goOn)
            {
                bool needsMoreDecoding = false;
                
                CHECK(packet != nullptr, "inconsistency error");
                goOn = decodePacket(packet, m_rawVideoFrame, gotFrame, needsMoreDecoding);
                
                if (gotFrame)
                {
                    rescale(m_rawVideoFrame, m_rgbaVideoBuffer, m_rgbaVideoLinesize);
                    texture.update(m_rgbaVideoBuffer[0]);
                }
                
                if (needsMoreDecoding)
                {
                    prependEncodedData(packet);
                }
                else
                {
                    av_free_packet(packet);
                    av_free(packet);
                }
                
                if (!gotFrame && goOn)
                {
                    sfeLogDebug("no image in this packet, reading further");
                    packet = popEncodedData();
                }
            }
        }
        
        return goOn;
    }
    
    sf::Time VideoStream::getSynchronizationGap()
    {
        return computePosition() - m_timer->getOffset();
    }
    
    bool VideoStream::decodePacket(AVPacket* packet, AVFrame* outputFrame, bool& gotFrame, bool& needsMoreDecoding)
    {
        int gotPicture = 0;
        needsMoreDecoding = false;
        
        int decodedLength = avcodec_decode_video2(m_stream->codec, outputFrame, &gotPicture, packet);
        gotFrame = (gotPicture != 0);
        
        if (decodedLength > 0 || gotFrame)
        {
            if (decodedLength < packet->size)
            {
                needsMoreDecoding = true;
                packet->data += decodedLength;
                packet->size -= decodedLength;
            }
            
            if (gotFrame)
            {
                int64_t timestamp = av_frame_get_best_effort_timestamp(outputFrame);
                int64_t startTime = m_stream->start_time != AV_NOPTS_VALUE ? m_stream->start_time : 0;
                sf::Int64 ms = 1000 * (timestamp - startTime) * av_q2d(m_stream->time_base);
                m_lastDecodedTimestamp = sf::milliseconds(ms);
            }
            
            return true;
        }
        else
        {
            return false;
        }
    }
    
    void VideoStream::initRescaler()
    {
        /* create scaling context */
        int algorithm = SWS_FAST_BILINEAR;
        
        if (getFrameSize().x % 8 != 0 && getFrameSize().x * getFrameSize().y < 500000)
        {
            algorithm |= SWS_ACCURATE_RND;
        }
        
        m_swsCtx = sws_getCachedContext(nullptr, m_stream->codec->width, m_stream->codec->height, m_stream->codec->pix_fmt,
                                        m_stream->codec->width, m_stream->codec->height, PIX_FMT_RGBA,
                                        algorithm, nullptr, nullptr, nullptr);
        CHECK(m_swsCtx, "VideoStream::initRescaler() - sws_getContext() error");
    }
    
    void VideoStream::rescale(AVFrame* frame, uint8_t* outVideoBuffer[4], int outVideoLinesize[4])
    {
        CHECK(frame, "VideoStream::rescale() - invalid argument");
        sws_scale(m_swsCtx, frame->data, frame->linesize, 0, frame->height, outVideoBuffer, outVideoLinesize);
    }
    
    void VideoStream::preload()
    {
        sfeLogDebug("Preload video image");
        onGetData(m_texture);
    }
    
    void VideoStream::willPlay(const Timer &timer)
    {
        Stream::willPlay(timer);
        if (getStatus() == Stopped)
        {
            preload();
        }
    }
}
