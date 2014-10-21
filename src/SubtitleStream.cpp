/*
 *  SubtitleStream.cpp
 *  sfeMovie project
 *
 *  Copyright (C) 2010-2014 Stephan Vedder
 *  stephan.vedder@gmail.com
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
}

#include <sfeMovie/Movie.hpp>
#include "SubtitleStream.hpp"
#include "Log.hpp"

#include <iostream>
#include <cassert>
#include <stdint.h>


namespace sfe
{
    const int RGBASize = 4;
    
    SubtitleStream::SubtitleStream(AVFormatContext* formatCtx, AVStream* stream, DataSource& dataSource, Timer& timer, Delegate& delegate) :
    Stream(formatCtx, stream, dataSource, timer), m_delegate(delegate)
    {
    }
    
    /** Default destructor
     */
    SubtitleStream::~SubtitleStream()
    {
        
    }
    
    MediaType SubtitleStream::getStreamKind() const
    {
        return Subtitle;
    }
    
    void SubtitleStream::update()
    {
        //only get new subtitles if we are running low
        if (m_status == Playing && hasPackets())
        {
            if (!onGetData())
                setStatus(Stopped);
        }
        
        if (m_pendingSubtitles.size() > 0)
        {
            //activate subtitle
            if (m_pendingSubtitles.front()->start < m_timer.getOffset())
            {
                SubtitleData* iter = m_pendingSubtitles.front();
                
                m_delegate.didUpdateSubtitle(*this, iter->sprites, iter->positions);
                m_visibleSubtitles.push_back(iter);
                m_pendingSubtitles.pop_front();
            }
        }
        
        
        if (m_visibleSubtitles.size()>0)
        {
            //remove subtitle
            if (m_visibleSubtitles.front()->end < m_timer.getOffset())
            {
                SubtitleData* subtitle = m_visibleSubtitles.front();
                m_visibleSubtitles.pop_front();
                
                if (m_visibleSubtitles.size() == 0)
                {
                    m_delegate.didUpdateSubtitle(*this, std::list<sf::Sprite>(), std::list<sf::Vector2i>());
                    delete subtitle;
                }
            }
        }
    }
    
    bool SubtitleStream::onGetData()
    {
        AVPacket* packet = popEncodedData();
        AVSubtitle sub;
        int32_t gotSub = 0;
        uint32_t goOn = 0;
        int64_t pts = 0;
        
        if (packet)
        {
            goOn = 1;
            
            while (!gotSub && packet && goOn)
            {
                bool needsMoreDecoding = false;
                
                CHECK(packet != NULL, "inconsistency error");
                goOn = avcodec_decode_subtitle2(m_codecCtx, &sub, &gotSub, packet);
                
                pts = 0;
                if (packet->pts != AV_NOPTS_VALUE)
                    pts = packet->pts;
                
                if (gotSub && pts)
                {
                    SubtitleData* sfeSub = new SubtitleData(&sub);
                    m_pendingSubtitles.push_back(sfeSub);
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
                
                if (!gotSub && goOn)
                {
                    sfeLogDebug("no subtitle in this packet, reading further");
                    packet = popEncodedData();
                }
            }
        }
        return static_cast<bool>(goOn);
    }
    
    
    SubtitleStream::SubtitleData::SubtitleData(AVSubtitle* sub)
    {
        assert(sub != NULL);
        
        start = sf::milliseconds(sub->start_display_time) + sf::microseconds(sub->pts);
        end = sf::milliseconds(sub->end_display_time) + sf::microseconds(sub->pts);
        
        for (int i = 0; i < sub->num_rects; ++i)
        {
            
            sprites.push_back(sf::Sprite());
            textures.push_back(sf::Texture());
            
            sf::Sprite& sprite = sprites.back();
            sf::Texture& texture = textures.back();
            AVSubtitleRect* subItem = sub->rects[i];
            
            AVSubtitleType type = subItem->type;
            
            if (type == SUBTITLE_BITMAP || subItem->w * subItem->h > 0)
            {
                positions.push_back(sf::Vector2i(subItem->x, subItem->y));
                uint32_t* palette = new uint32_t[subItem->nb_colors];
                for (int j = 0; j < subItem->nb_colors; j++)
                    palette[j] = *(uint32_t*)&subItem->pict.data[1][j * RGBASize];
                
                texture.create(subItem->w, subItem->h);
                texture.setSmooth(true);
                
                uint32_t* data = new uint32_t[subItem->w* sub->rects[i]->h];
                for (int j = 0; j < subItem->w * subItem->h; ++j)
                    data[j] = palette[subItem->pict.data[0][j]];
                
                texture.update((uint8_t*)data);
                sprite.setTexture(texture);
                
                delete[] data;
                delete[] palette;
            }
            else
            {
                //TODO: add libass code
                if (subItem->text != NULL)
                {
                    if (subItem->type == SUBTITLE_TEXT)
                        sfeLogError("Unsupported subtitle type: it would require text support");
                    else
                        sfeLogError("Unsupported subtitle type: it can be approximated with text");
                }
                else if (subItem->ass != NULL)
                {
                    if (subItem->type == SUBTITLE_ASS)
                        sfeLogError("Unsupported subtitle type: it would require ASS support");
                    else
                        sfeLogError("Unsupported subtitle type: it could be rendered with ASS support");
                }
            }
        }
    }
    
    void SubtitleStream::didStop(const Timer& timer, sfe::Status previousStatus)
    {
        while (m_visibleSubtitles.size())
        {
            delete m_visibleSubtitles.back();
            m_visibleSubtitles.pop_back();
        }
        
        while (m_pendingSubtitles.size())
        {
            delete m_pendingSubtitles.back();
            m_pendingSubtitles.pop_back();
        }
        
        Stream::didStop(timer, previousStatus);
    }
}
