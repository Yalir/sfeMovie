
/*
 *  SubtitleStream.cpp
 *  sfeMovie project
 *
 *  Copyright (C) 2010-2015 Stephan Vedder
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
#include <cstdint>

namespace sfe
{
    namespace
    {
#ifdef SFEMOVIE_ENABLE_ASS_SUBTITLES
        void ass_log(int ass_level, const char *fmt, va_list args, void *data)
        {
            char buffer[4096];
            
            vsnprintf(buffer, sizeof(buffer), fmt, args);
            sfeLogDebug(buffer);
        }
#endif
    }
    
    const int RGBASize = 4;
    
    SubtitleStream::SubtitleStream(AVFormatContext*& formatCtx, AVStream*& stream, DataSource& dataSource, std::shared_ptr<Timer> timer, Delegate& delegate) :
        Stream(formatCtx, stream, dataSource, timer),
        m_delegate(delegate),
        m_library(nullptr),
        m_renderer(nullptr),
        m_track(nullptr)
    {
        const AVCodecDescriptor* desc = av_codec_get_codec_descriptor(m_stream->codec);
        CHECK(desc != NULL, "Could not get the codec descriptor!");
        
        if((desc->props & AV_CODEC_PROP_BITMAP_SUB) == 0)
        {
#ifdef SFEMOVIE_ENABLE_ASS_SUBTITLES
			m_library  = ass_library_init();
            ass_set_message_cb(m_library, ass_log, nullptr);

			m_renderer = ass_renderer_init(m_library);
            m_track    = ass_new_track(m_library);
            
            ass_set_fonts(m_renderer, NULL, NULL , 1, NULL, 1);
            ass_set_margins(m_renderer, 10, 0, 0, 0);

            ass_process_codec_private(m_track, reinterpret_cast<char*>(m_stream->codec->subtitle_header), m_stream->codec->subtitle_header_size);
#else
            throw std::runtime_error("Non-bitmap subtitle stream detected but ASS support is disabled. Cannot use stream.");
#endif
		}
    }
    
    SubtitleStream::~SubtitleStream()
    {
#ifdef SFEMOVIE_ENABLE_ASS_SUBTITLES
        if (m_track)
        {
            ass_free_track(m_track);
            m_track = nullptr;
        }

		if(m_renderer)
		{
			ass_renderer_done(m_renderer);
			m_renderer = nullptr;
		}
		
		if(m_library)
		{
			ass_library_done(m_library);
			m_library = nullptr;
		}
#endif
    }
    
    void SubtitleStream::setRenderingFrame(int width, int height)
    {
#ifdef SFEMOVIE_ENABLE_ASS_SUBTITLES
        ass_set_frame_size(m_renderer, width, height);
#endif
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

#ifdef SFEMOVIE_ENABLE_ASS_SUBTITLES
                //this is the case for ass subtitles
                if (iter->type==ASS)
                {
                    int change = 0;
                    ASS_Image* img = ass_render_frame(m_renderer, m_track, m_timer->getOffset().asMilliseconds(), &change);
                   
                    if (change)
                    {                     

                        for (; img; img->next)
                        {
                            uint8_t     r = img->color >> 24,
                                        g = img->color >> 16 & 255,
                                        b = img->color >> 8 & 255,
                                        a = 255 - (img->color & 255);
                            
                            sf::Image buffer;
                            buffer.create(img->w, img->h);

                            iter->positions.push_back(sf::Vector2i(img->dst_x, img->dst_y));
                            
                            for (int y = 0; y < img->h; ++y)
                            {
                                const unsigned char *map = img->bitmap + y * img->stride;

                                for (int x = 0; x < img->w; ++x)
                                {
                                    uint8_t alpha = ((unsigned)a * *map++)/255;
                                    buffer.setPixel(x, y, sf::Color(r,g,b,alpha));
                                }
                            }

                            img = img->next;

                            iter->sprites.push_back(sf::Sprite());
                            iter->textures.push_back(sf::Texture());

                            
                            sf::Sprite& sprite = iter->sprites.back();
                            sf::Texture& texture = iter->textures.back();

                            texture.create(buffer.getSize().x, buffer.getSize().y);
                            texture.update(buffer);
                            texture.setSmooth(true);

                            sprite.setTexture(texture);
                        }                       
                    }
                }
#endif

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
        int32_t goOn = 0;
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
                    std::shared_ptr<SubtitleData> sfeSub = std::make_shared<SubtitleData>(&sub, succeeded,m_track);
                    
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
        return (goOn >= 0);
    }
    
    
    SubtitleStream::SubtitleData::SubtitleData(AVSubtitle* sub, bool& succeeded,ASS_Track* track)
    {
        assert(sub != nullptr);
        
        succeeded = false;
        start = sf::milliseconds(sub->start_display_time) + sf::microseconds(sub->pts);
        end = sf::milliseconds(sub->end_display_time) + sf::microseconds(sub->pts);

        for (unsigned int i = 0; i < sub->num_rects; ++i)
        {           
            AVSubtitleRect* subItem = sub->rects[i];

            if (subItem->type == SUBTITLE_BITMAP)
            {
                type = BITMAP;
                sprites.push_back(sf::Sprite());
                textures.push_back(sf::Texture());

                sf::Sprite& sprite = sprites.back();
                sf::Texture& texture = textures.back();

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
#ifdef SFEMOVIE_ENABLE_ASS_SUBTITLES
            else
            {
                type = ASS;
                ass_process_data(track, subItem->ass, strlen(subItem->ass));

                succeeded = true;
            }
#endif
        }
    }
    
    void SubtitleStream::flushBuffers()
    {
        m_delegate.didWipeOutSubtitles(*this);
        Stream::flushBuffers();
        m_pendingSubtitles.clear();
        m_visibleSubtitles.clear();
    }
    
    bool SubtitleStream::fastForward(sf::Time targetPosition)
    {
        while(hasPackets())
        {
            onGetData();
        }

		std::list< std::shared_ptr<SubtitleData>>::iterator it = m_visibleSubtitles.begin();
		while (it != m_visibleSubtitles.end()) 
		{
			//erase subs that are deleted before the targetPosition
			if (it->get()->end<targetPosition)
				it = m_visibleSubtitles.erase(it);
			else
				++it;
		}
		
		it = m_pendingSubtitles.begin();
		while (it != m_pendingSubtitles.end()) 
		{
			//erase subs that are deleted before the targetPosition
			if (it->get()->end<targetPosition)
				it = m_pendingSubtitles.erase(it);
			else
				++it;
		}
        
        return true;
	}
}
