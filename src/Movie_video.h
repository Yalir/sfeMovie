/*
 *  Movie_video.h
 *  SFE (SFML Extension) project
 *
 *  Copyright (C) 2010-2011 Soltic Lucas
 *  soltic.lucas@gmail.com
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

#ifndef MOVIE_VIDEO_H
#define MOVIE_VIDEO_H

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <queue>


namespace sfe {
	class Movie;
	class Movie_video {
		public:
			Movie_video(Movie& parent);
			~Movie_video(void);
			
			// -------------------------- Video methods ----------------------------
			bool Initialize(void);
			void Play(void);
			void Pause(void);
			void Stop(void);
			void Close(void);
			
			void Render(sf::RenderTarget& Target) const;
			
			int GetStreamID(void) const;
			const sf::Vector2i& GetSize(void) const;
			float GetWantedFrameTime(void) const;
			sf::Image GetImageCopy(void) const;
			
			void Run(void); // thread callback
			void SetPlayingOffset(float time);
			void SkipFrames(unsigned count);
			void LoadNextImage(void);
			bool ReadFrame(void);
			bool HasPendingDecodableData(void);
			void DecodeFrontFrame(void);
			void PushFrame(AVPacket *pkt);
			void PopFrame(void);
			AVPacket *FrontFrame(void);
			void WatchThread(void);
			
			// ------------------------- Video attributes --------------------------
			Movie& m_parent;
			
			AVCodecContext *m_codecCtx;
			AVCodec *m_codec;
			AVFrame *m_rawFrame;
			AVFrame *m_RGBAFrame;
			std::queue <AVPacket *> m_packetList;
			sf::Mutex m_packetListMutex;
			struct SwsContext *m_swsCtx;
			
			sf::Thread m_thread;
			sf::Thread m_threadWatcher;
			//sf::Context m_context; // No more needed as of latest version of SFML 2.x

			int m_streamID;
			sf::Uint8 *m_pictureBuffer; // Buffer used to convert image from pixel matrix to simple array
			mutable sf::Mutex m_imageMutex;
			sf::Image m_image;
			bool m_isLate;
			sf::Sprite m_sprite;
			float m_wantedFrameTime;
			unsigned m_displayedFrameCount;
			float m_decodingTime;
			sf::Clock m_timer;
			bool m_runThread;
			
			sf::Vector2i m_size;
		};
	} // namespace sfe
	
#endif
