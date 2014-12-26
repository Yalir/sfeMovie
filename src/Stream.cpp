
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
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include "Stream.hpp"
#include "Utilities.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>

namespace sfe
{
    Stream::Stream(AVFormatContext*& formatCtx, AVStream*& stream, DataSource& dataSource, std::shared_ptr<Timer> timer) :
    m_formatCtx(formatCtx),
    m_stream(stream),
    m_dataSource(dataSource),
    m_timer(timer),
    m_codec(nullptr),
    m_streamID(-1),
    m_packetList(),
    m_status(Stopped),
    m_readerMutex()
    {
        CHECK(stream, "Stream::Stream() - invalid stream argument");
        CHECK(timer, "Inconcistency error: null timer");
        int err = 0;
        
        m_stream = stream;
        m_streamID = stream->index;
        CHECK(m_stream, "Inconcistency error: null stream")
        CHECK(m_streamID >= 0, "Inconcistency error: invalid stream id");
        
        // Get the decoder
        m_codec = avcodec_find_decoder(m_stream->codec->codec_id);
        CHECK(m_codec, "Stream() - no decoder for " + std::string(avcodec_get_name(m_stream->codec->codec_id)) + " codec");
        
        // Load the codec
        err = avcodec_open2(m_stream->codec, m_codec, nullptr);
        CHECK0(err, "Stream() - unable to load decoder for codec " + std::string(avcodec_get_name(m_stream->codec->codec_id)));
        
        AVDictionaryEntry* entry = av_dict_get(m_stream->metadata, "language", nullptr, 0);
        if (entry)
        {
            m_language = entry->value;
        }
    }
    
    Stream::~Stream()
    {
        disconnect();
        flushBuffers();
        
        if (m_formatCtx && m_stream && m_stream->codec)
        {
            avcodec_close(m_stream->codec);
        }
    }
    
    void Stream::connect()
    {
        m_timer->addObserver(*this);
    }
    
    void Stream::disconnect()
    {
        m_timer->removeObserver(*this);
    }
    
    void Stream::pushEncodedData(AVPacket* packet)
    {
        CHECK(packet, "invalid argument");
        sf::Lock l(m_readerMutex);
        m_packetList.push_back(packet);
    }
    
    void Stream::prependEncodedData(AVPacket* packet)
    {
        CHECK(packet, "invalid argument");
        sf::Lock l(m_readerMutex);
        m_packetList.push_front(packet);
    }
    
    AVPacket* Stream::popEncodedData()
    {
        AVPacket* result = nullptr;
        sf::Lock l(m_readerMutex);
        
        if (!m_packetList.size() && !isPassive())
        {
            m_dataSource.requestMoreData(*this);
        }
        
        if (m_packetList.size())
        {
            result = m_packetList.front();
            m_packetList.pop_front();
        }
        else
        {
            if (m_stream->codec->codec->capabilities & CODEC_CAP_DELAY)
            {
                AVPacket* flushPacket = (AVPacket*)av_malloc(sizeof(*flushPacket));
                av_init_packet(flushPacket);
                flushPacket->data = nullptr;
                flushPacket->size = 0;
                result = flushPacket;
                
                sfeLogDebug("Sending flush packet: " + mediaTypeToString(getStreamKind()));
            }
        }
        
        return result;
    }
    
    void Stream::flushBuffers()
    {
        sf::Lock l(m_readerMutex);
        if (getStatus() == Playing)
        {
            sfeLogWarning("packets flushed while the stream is still playing");
        }
        
        if (m_formatCtx && m_stream)
            avcodec_flush_buffers(m_stream->codec);
        
        AVPacket* pkt = nullptr;
        
        while (m_packetList.size())
        {
            pkt = m_packetList.front();
            m_packetList.pop_front();
            
            av_free_packet(pkt);
            av_free(pkt);
        }
        
        sfeLogDebug("Flushed " + mediaTypeToString(getStreamKind()) + " stream!");
    }
    
    bool Stream::needsMoreData() const
    {
        return m_packetList.size() < 10;
    }
    
    MediaType Stream::getStreamKind() const
    {
        return Unknown;
    }
    
    Status Stream::getStatus() const
    {
        return m_status;
    }
    
    std::string Stream::getLanguage() const
    {
        return m_language;
    }
    
    sf::Time Stream::computePosition()
    {
        if (!m_packetList.size())
        {
            m_dataSource.requestMoreData(*this);
        }
        
        if (!m_packetList.size())
        {
            return sf::Time::Zero;
        }
        else
        {
            AVPacket* packet = *m_packetList.begin();
            CHECK(packet, "internal inconcistency");
            
            int64_t timestamp = -424242;
            
            if (packet->dts != AV_NOPTS_VALUE)
            {
                timestamp = packet->dts;
            }
            else if (packet->pts != AV_NOPTS_VALUE)
            {
                int64_t startTime = m_stream->start_time != AV_NOPTS_VALUE ? m_stream->start_time : 0;
                timestamp = packet->pts - startTime;
            }
            
            return sf::seconds(timestamp * av_q2d(m_stream->time_base));
        }
    }
    
    bool Stream::canUsePacket(AVPacket* packet) const
    {
        CHECK(packet, "inconcistency error: null argument");
        
        return packet->stream_index == m_stream->index;
    }
    
    bool Stream::isPassive() const
    {
        return false;
    }
    
    void Stream::setStatus(Status status)
    {
        m_status = status;
    }
    
    void Stream::didPlay(const Timer& timer, Status previousStatus)
    {
        setStatus(Playing);
    }
    
    void Stream::didPause(const Timer& timer, Status previousStatus)
    {
        setStatus(Paused);
    }
    
    void Stream::didStop(const Timer& timer, Status previousStatus)
    {
        setStatus(Stopped);
    }
    
    void Stream::willSeek(const Timer& timer, sf::Time position)
    {
    }
    
    void Stream::didSeek(const Timer& timer, sf::Time position)
    {
        flushBuffers();
    }
    
    bool Stream::hasPackets()
    {
        return m_packetList.size() > 0;
    }
}
