
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
    
    SubtitleStream::SubtitleStream(AVFormatContext*& formatCtx, AVStream*& stream, DataSource& dataSource, std::shared_ptr<Timer> timer, Delegate& delegate) :
    Stream(formatCtx, stream, dataSource, timer), m_delegate(delegate)
    {
        const AVCodecDescriptor* desc = av_codec_get_codec_descriptor(m_stream->codec);
        CHECK(desc != NULL, "Could not get the codec descriptor!");
        CHECK((desc->props & AV_CODEC_PROP_BITMAP_SUB) != 0,
              "Subtitle stream doesn't provide bitmap subtitles, this is not supported yet!"
              "\nSee https://github.com/Yalir/sfeMovie/issues/7");
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
            if (m_pendingSubtitles.front()->start < m_timer->getOffset())
            {
                std::shared_ptr<SubtitleData> iter = m_pendingSubtitles.front();
                
                m_delegate.didUpdateSubtitle(*this, iter->sprites, iter->positions);
                m_visibleSubtitles.push_back(iter);
                m_pendingSubtitles.pop_front();
            }
        }
        
        
        if (m_visibleSubtitles.size()>0)
        {
            //remove subtitle
            if (m_visibleSubtitles.front()->end < m_timer->getOffset())
            {
                std::shared_ptr<SubtitleData> subtitle = m_visibleSubtitles.front();
                m_visibleSubtitles.pop_front();
                
                if (m_visibleSubtitles.size() == 0)
                {
                    m_delegate.didWipeOutSubtitles(*this);
                }
            }
        }
    }
    
    bool SubtitleStream::isPassive() const
    {
        return true;
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
                
                CHECK(packet != nullptr, "inconsistency error");
                goOn = avcodec_decode_subtitle2(m_stream->codec, &sub, &gotSub, packet);
                
                pts = 0;
                if (packet->pts != AV_NOPTS_VALUE)
                    pts = packet->pts;
                
                if (gotSub && pts)
                {
                    bool succeeded = false;
                    std::shared_ptr<SubtitleData> sfeSub = std::make_shared<SubtitleData>(&sub, succeeded);
                    
                    if (succeeded)
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
        return (goOn != 0);
    }
    
    
    SubtitleStream::SubtitleData::SubtitleData(AVSubtitle* sub, bool& succeeded)
    {
        assert(sub != nullptr);
        
        succeeded = false;
        start = sf::milliseconds(sub->start_display_time) + sf::microseconds(sub->pts);
        end = sf::milliseconds(sub->end_display_time) + sf::microseconds(sub->pts);
        
        for (unsigned int i = 0; i < sub->num_rects; ++i)
        {
            
            sprites.push_back(sf::Sprite());
            textures.push_back(sf::Texture());
            
            sf::Sprite& sprite = sprites.back();
            sf::Texture& texture = textures.back();
            AVSubtitleRect* subItem = sub->rects[i];
            
            AVSubtitleType type = subItem->type;
            
            if (type == SUBTITLE_BITMAP)
            {
                CHECK(subItem->pict.data != nullptr, "FFmpeg inconcistency error");
                CHECK(subItem->w * subItem->h > 0, "FFmpeg inconcistency error");
                
                positions.push_back(sf::Vector2i(subItem->x, subItem->y));
                
                std::unique_ptr<uint32_t[]> palette(new uint32_t[subItem->nb_colors]);
                for (int j = 0; j < subItem->nb_colors; j++)
                    palette[j] = *(uint32_t*)&subItem->pict.data[1][j * RGBASize];
                
                texture.create(subItem->w, subItem->h);
                texture.setSmooth(true);
                
                std::unique_ptr<uint32_t[]> data(new uint32_t[subItem->w* sub->rects[i]->h]);
                for (int j = 0; j < subItem->w * subItem->h; ++j)
                    data[j] = palette[subItem->pict.data[0][j]];
                
                texture.update((uint8_t*)data.get());
                sprite.setTexture(texture);
                
                succeeded = true;
            }
            else
            {
                //TODO: add libass code
                if (subItem->text != nullptr)
                {
                    if (subItem->type == SUBTITLE_TEXT)
                        sfeLogError("Unsupported subtitle type: it would require text support");
                    else
                        sfeLogError("Unsupported subtitle type: it can be approximated with text");
                }
                else if (subItem->ass != nullptr)
                {
                    if (subItem->type == SUBTITLE_ASS)
                        sfeLogError("Unsupported subtitle type: it would require ASS support");
                    else
                        sfeLogError("Unsupported subtitle type: it could be rendered with ASS support");
                }
            }
        }
    }
    
    void SubtitleStream::flushBuffers()
    {
        m_delegate.didWipeOutSubtitles(*this);
        Stream::flushBuffers();
    }
}
