
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
	const int RGBASize = 4;

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
		return Subtitle;
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

		if(m_inactive.size() > 0)
		{
			//activate subtitle
			if (m_inactive.front()->start < m_timer.getOffset())
			{
				SubtitleData* iter = m_inactive.front();
				m_delegate.didUpdateSubtitle(*this, iter->sprites);
				m_active.push_back(iter);
				m_inactive.pop_front();
			}
		}
					

		if (m_active.size()>0)
		{
			//remove subtitle
			if (m_active.front()->end < m_timer.getOffset())
			{
				m_active.pop_front();
				//m_active.erase(m_active.begin() + i);
				if (m_active.size() == 0)
				{
					std::vector<sf::Sprite> empty;
					m_delegate.didUpdateSubtitle(*this, empty);
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
					SubtitleData* sfeSub = new SubtitleData(&sub);
					m_inactive.push_back(sfeSub);
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


	SubtitleStream::SubtitleData::SubtitleData(AVSubtitle* sub)
	{
		start = sf::Time(sf::milliseconds(sub->start_display_time) + sf::microseconds(sub->pts));
		end = sf::Time(sf::milliseconds(sub->end_display_time) + sf::microseconds(sub->pts));

		for (int i = 0; i < sub->num_rects; ++i)
		{
			sprites.push_back(sf::Sprite());
			sf::Sprite& sprite = sprites.back();
			AVSubtitleRect* cRect = sub->rects[i];
			sprite.setOrigin(sf::Vector2f(cRect->x, cRect->y));
			sprite.setPosition(sf::Vector2f(cRect->x * 2, cRect->y * 1.75f));
			uint32_t* palette = new uint32_t[cRect->nb_colors];
			for (int j = 0; j < cRect->nb_colors; j++)
			{
				palette[j] = *(uint32_t*)&cRect->pict.data[1][j * RGBASize];
			}

			textures.push_back(sf::Texture());
			sf::Texture& tex = textures.back();
			tex.create(cRect->w, cRect->h);

			uint32_t* data = new uint32_t[cRect->w* sub->rects[i]->h];
			for (int j = 0; j <cRect->w* cRect->h; ++j)
			{
				data[j] = palette[cRect->pict.data[0][j]];
			}
			tex.update((uint8_t*)data);
			sprite.setTexture(tex);
			
			delete[] data;
			delete[] palette;
		}
	}
}