
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

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#include <cstring>
#include <iostream>
#include "SubtitleStream.hpp"
#include "Log.hpp"
#include <sfeMovie/Movie.hpp>


namespace sfe {
	SubtitleStream::SubtitleStream(AVFormatContext* formatCtx, AVStream* stream, DataSource& dataSource, Timer& timer,Delegate& delegate) :
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
		return MEDIA_TYPE_SUBTITLE;
	}

	void SubtitleStream::update()
	{
		//only get new subtitles if we are running low
		if (m_status == Playing && hasPackets())
		{
				if (!onGetData()) {
				setStatus(Stopped);
			}
		}

		for (uint32_t i = 0; i < m_inactive.size(); ++i)
		{
			//activate subtitle
			if (m_inactive[i].pts + m_inactive[i].sub.start_display_time < m_timer.getOffset().asMilliseconds())
			{
				SubImage& c = m_inactive[i];	
				c.out = SubtitleToSprites(&c.sub);
				m_delegate.didUpdateSubtitle(*this, c.out);
				m_active.push_back(c);
				m_inactive.erase(m_inactive.begin() + i);
				
				
			}
		}

		for (uint32_t i = 0; i < m_active.size(); ++i)
		{
			if (m_active[i].pts + m_active[i].sub.end_display_time < m_timer.getOffset().asMilliseconds())
			{
				m_active.erase(m_active.begin() + i);
				if (m_active.size() == 0)
				{
					std::vector<sf::Sprite> empty;
					m_delegate.didUpdateSubtitle(*this, empty);
					m_textures.clear();
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

		if (packet) {
			goOn = 1;

			while (!gotSub && packet && goOn) {
				bool needsMoreDecoding = false;

				CHECK(packet != NULL, "inconsistency error");
				goOn = avcodec_decode_subtitle2(m_codecCtx, &sub, &gotSub, packet);

				pts = 0;
				if (packet->pts != AV_NOPTS_VALUE)
					pts =  packet->pts;

				if (gotSub && pts) {
					SubImage subimg;
					subimg.sub = sub;
					subimg.pts = pts;
					
					m_inactive.push_back(subimg);
				}

				if (needsMoreDecoding) {
					prependEncodedData(packet);
				}
				else {
					av_free_packet(packet);
					av_free(packet);
				}

				if (!gotSub && goOn) {
					sfeLogDebug("no subtitle in this packet, reading further");
					packet = popEncodedData();
				}
			}
		}

		return static_cast<bool>(goOn);
	}

	std::vector<sf::Sprite> SubtitleStream::SubtitleToSprites(AVSubtitle* sub)
	{
		std::vector<sf::Sprite> sprites;

		for (int i = 0; i < sub->num_rects; ++i)
		{
			sf::Sprite sprite;
			AVSubtitleRect* cRect = sub->rects[i];
			sprite.setOrigin(sf::Vector2f(cRect->x, cRect->y));
			sprite.setPosition(sf::Vector2f(cRect->x * 2, cRect->y * 1.75f));
			uint32_t* palette = new uint32_t[cRect->nb_colors];
			for (int j = 0; j < cRect->nb_colors; j++)
			{
				palette[j] = *(uint32_t*)&cRect->pict.data[1][j * kRgbaSize];
			}
			

			m_textures.push_back(sf::Texture());
			sf::Texture& tex = m_textures.back();
			tex.create(cRect->w, cRect->h);

			uint32_t* data = new uint32_t[cRect->w* sub->rects[i]->h];
			for (int j = 0; j <cRect->w* cRect->h; ++j)
			{
				data[j] = palette[cRect->pict.data[0][j]];
			}
			tex.update((uint8_t*)data);
			sprite.setTexture(tex);
			sprites.push_back(sprite);
			delete[] data;
			delete[] palette;
		}

		return sprites;
	}
}