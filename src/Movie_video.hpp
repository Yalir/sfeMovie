/*
 *  Movie_video.hpp
 *  sfeMovie project
 *
 *  Copyright (C) 2010-2012 Lucas Soltic
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

#ifndef MOVIE_VIDEO_HPP
#define MOVIE_VIDEO_HPP

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <queue>
#include "Condition.hpp"


namespace sfe {
	class Movie;
	class Movie_video {
	public:
		Movie_video(Movie& parent);
		~Movie_video(void);
		
		// -------------------------- Video methods ----------------------------
		bool initialize(void);
		void play(void);
		void pause(void);
		void stop(void);
		void close(void);
		
		void draw(sf::RenderTarget& Target, sf::RenderStates& state) const;
		
		int getStreamID(void) const;
		const sf::Vector2i& getSize(void) const;
		sf::Time getWantedFrameTime(void) const;
		const sf::Texture& getCurrentFrame(void) const;
		void ensureTextureUpdate(void) const;
		
		void update(void); // Swaping and synching thread
		void decode(void); // Decoding thread
		
		bool getLateState(sf::Time& waitTime) const;
		bool isStarving(void);
		void setPlayingOffset(sf::Time time);
		//void SkipFrames(unsigned count);
		
		void swapImages(bool unconditionned = false);
		sf::Texture& frontTexture(void);
		const sf::Texture& frontTexture(void) const;
		sf::Texture& backTexture(void);
		
		bool preLoad(void);
		bool loadNextImage(bool isLate);
		bool readFrame(void);
		bool hasPendingDecodableData(void);
		bool decodeFrontFrame(bool isLate);
		void pushFrame(AVPacket *pkt);
		void popFrame(void);
		AVPacket *frontFrame(void);
		void watchThread(void);
		
	private:
		// ------------------------- Video attributes --------------------------
		Movie& m_parent;			// Link to the parent movie
		
		// Image and decoding stuff
		AVCodecContext *m_codecCtx; // Decoder information
		AVCodec *m_codec;			// Video decoder
		AVFrame *m_rawFrame;		// Original YUV422 frame
		mutable AVFrame *m_frontRGBAFrame;	// Front converted RGBA frame
		mutable AVFrame *m_backRGBAFrame;	// Back converted RGBA frame
		int m_streamID;				// The video stream identifier in the video file
		sf::Uint8 *m_pictureBuffer; // Buffer used to convert image from pixel matrix to simple array
		struct SwsContext *m_swsCtx;// Used for converting image from YUV422 to RGBA
		
		// Packets' queueing stuff
		std::queue <AVPacket *> m_packetList;// Awaiting video packets (that will be decoded later)
		sf::Mutex m_packetListMutex;// Prevent packets' list from being/modified and accessed at the same time from several threads
		
		// Threads
		//sf::Thread m_updateThread;	// Does swaping and time sync
		sf::Thread m_decodeThread;	// Does video decoding
		Condition m_running;
		
		// Image swaping
		mutable sf::Mutex m_imageSwapMutex;// Prevent the textures from being swaped while being updated
		mutable Condition m_backImageReady;	// condition to wait until the image is ready for swaping
		//mutable bool m_isBackFrameReady;
		unsigned m_imageIndex;		// To know which image is the front or back one
		mutable sf::Texture m_tex;			// The image in VRAM
		sf::Sprite m_sprite;		// Sprite bound to the front image
		sf::Vector2i m_size;		// The images size
		
		// Miscellaneous parameters
		bool m_isStarving;			// If true, there is no more video packet to read and decode
		sf::Time m_wantedFrameTime;	// For how long should one frame last
		unsigned m_displayedFrameCount;// How many frames did we display? (and guess whether we're late)
		sf::Time m_decodingTime;	// How long does it take to decode one frame? (used to know more precisely when we should decode and swap)
		sf::Clock m_timer;			// Used to compute the decoding time
		bool m_runThread;			// Should the updating and decoding still run?
	};
} // namespace sfe

#endif
