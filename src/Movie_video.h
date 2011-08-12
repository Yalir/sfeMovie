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
#include "Condition.h"

namespace sfe {
	class Movie;
	class Movie_video {
	public:
		Movie_video(Movie& parent);
		~Movie_video(void);
		
		// Initial load and cleaning
		bool Initialize(void);
		void Close(void);
		
		// Movie playback controls
		void Play(void);
		void Pause(void);
		void Stop(void);
		void SetPlayingOffset(sf::Uint32 time);
		
		// sf::Drawable rendering
		void Render(sf::RenderTarget& Target) const;
		void DisplayNextImage(bool unconditionned = false);
		
		// Get some information
		int GetStreamID(void) const;
		const sf::Vector2i& GetSize(void) const;
		float GetWantedFrameTime(void) const;
		sf::Image GetImageCopy(void) const;
		
		// Video threads
		void UpdateThreadCallback(void); // Swaping and synching thread
		void DecodeThreadCallback(void); // Decoding thread
		
		// Get some states
		bool IsLate(void);
		bool IsLate(sf::Uint32& availableSleepTime);
		unsigned LateFramesCount(void);
		bool IsStarving(void);
		
		//void SkipFrames(unsigned count);

		// Image loading
		bool PreLoad(void);
		bool LoadNextImage(bool skipUpload = false);
		bool DecodeFrontFrame(bool skipUpload);
		
		// Raw frames (non-decoded) storing
		void PushFrame(AVPacket *pkt);
		bool ReadAndPushFrame(void);
		void PopFrame(void);
		AVPacket *FrontFrame(void);
		bool HasPendingFrame(void);
		
		// Textures storing
		void PushTexture(sf::Texture *tex);
		void PopTexture(void);
		void PopTexture_unlocked(void);
		sf::Texture& FrontTexture(void);
		sf::Texture& FrontTexture_unlocked(void);
		const sf::Texture& FrontTexture(void) const;
		unsigned ReadyTexturesQueueLength(void) const;
		unsigned ReadyTexturesQueueLength_unlocked(void) const;
		bool CanStoreMoreTextures(void) const;
		
		sf::Texture *TakeFreeTexture(void);
		
	private:
		// ------------------------- Video attributes --------------------------
		Movie& m_parent;			// Link to the parent movie
		
		// Image and decoding stuff
		AVCodecContext *m_codecCtx; // Decoder information
		AVCodec *m_codec;			// Video decoder
		AVFrame *m_rawFrame;		// Original YUV422 frame
		AVFrame *m_RGBAFrame;		// Converted RGBA frame
		int m_streamID;				// The video stream identifier in the video file
		sf::Uint8 *m_pictureBuffer; // Buffer used to convert image from pixel matrix to simple array
		struct SwsContext *m_swsCtx;// Used for converting image from YUV422 to RGBA
		
		// Packets' queueing stuff
		std::queue <AVPacket *> m_packetList;// Awaiting video packets (that will be decoded later)
		sf::Mutex m_packetListMutex;// Prevent packets' list from being/modified and accessed at the same time from several threads
		
		// Threads
		sf::Thread m_updateThread;	// Does swaping and time sync
		sf::Thread m_decodeThread;	// Does video decoding
		Condition m_running;
		
		// Image display
		mutable sf::Mutex m_texturesQueueMutex;// Prevent textures queue from being accessed and updated at the same time
		mutable sf::Mutex m_freeTexturesQueueMutex;
		mutable sf::Mutex m_imageMutex;
		sf::Texture *m_displayedTexture; // The texture currently being displayed
		std::queue<sf::Texture *> m_freeTexturesQueue; // Textures waiting to be updated (empty or with old images)
		std::queue<sf::Texture *> m_readyTexturesQueue; // Textures ready to be displayed (but waiting)
		Condition m_canDecodeOneMoreTexture;
		Condition m_hasImageReadyForDisplay;
		sf::Sprite m_sprite;		// Sprite bound to the front image
		sf::Vector2i m_size;		// The images size
		bool m_smooth;				// Should we smooth the textures
		
		// Miscellaneous parameters
		bool m_isLate;				// If true, we should skip some steps to catch up with the movie timeline
		bool m_isStarving;			// If true, there is no more video packet to read and decode
		float m_wantedFrameTime;	// For how long should one frame last
		unsigned m_displayedFrameCount;// How many frames did we display? (and guess whether we're late)
		unsigned m_loadedFrameCount;
		sf::Uint32 m_decodingTime;	// How long does it take to decode one frame? (used to know more precisely when we should decode and swap)
		sf::Clock m_timer;			// Used to compute the decoding time
	};
} // namespace sfe

#endif
