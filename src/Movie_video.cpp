/*
 *  Movie_video.cpp
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



#include "Movie_video.h"
#include "Movie.h"

#include "Movie_audio.h"
#include <SFML/Config.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <iostream>
#include <cassert>
#include "utils.h"

#define NTSC_FRAMERATE 29.97f
#define MAXIMUM_QUEUE_LENGTH 10
#define CRITICAL_QUEUE_LENGTH 2
#define ALLOWED_LATE_FRAMES_COUNT 3

#define SKIP_TEXTURE_UPLOAD true

namespace sfe {
	
	Movie_video::Movie_video(Movie& parent) :
	m_parent(parent),
	
	// Image and decoding stuff
	m_codecCtx(NULL),
	m_codec(NULL),
	m_rawFrame(NULL),
	m_RGBAFrame(NULL),
	m_streamID(-1),
	m_pictureBuffer(NULL), // Buffer used to convert image from pixel matrix to simple array
	m_swsCtx(NULL),
	
	// Packets' queueing stuff
	m_packetList(),
	// TODO: m_packetList should be replaced with a C-like lock-free and thread-safe queue
	m_packetListMutex(), 
	
	// Threads
	m_updateThread(&Movie_video::UpdateThreadCallback, this),	// Does swaping and time sync
	m_decodeThread(&Movie_video::DecodeThreadCallback, this),	// Does video decoding
	m_running(),
	
	// Image swaping
	//m_imageSwapMutex(),
	//m_backImageReady(),
	//m_imageIndex(0),
	//m_tex1(),
	//m_tex2(),
	
	// Miscellaneous parameters
	m_isLate(false),
	m_isStarving(false),
	m_sprite(),
	m_wantedFrameTime(0.f),
	m_displayedFrameCount(0),
	m_decodingTime(0),
	m_timer(),
	//m_runThread(false),
	m_size(0, 0),
	m_smooth(false)
	{
		
	}
	
	
	Movie_video::~Movie_video(void)
	{
	}
	
	
#pragma mark -
#pragma mark Initial load and cleaning
	////////////////////////////////////////////////////////////////////////////
	/// Initial load and cleaning
	////////////////////////////////////////////////////////////////////////////
	bool Movie_video::Initialize(void)
	{
		int err;
		
		// Find the video stream among the differents streams
		for (int i = 0; -1 == m_streamID && i < m_parent.GetAVFormatContext()->nb_streams; i++)
		{
			if (m_parent.GetAVFormatContext()->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO)
				m_streamID = i;
		}
		
		// If no video stream found...
		if (-1 == m_streamID)
		{
			Close();
			return false;
		}
		
		// Get the video codec
		m_codecCtx = m_parent.GetAVFormatContext()->streams[m_streamID]->codec;
		if (!m_codecCtx)
		{
			std::cerr << "Movie_video::Initialize() - unable to get the video codec context" << std::endl;
			Close();
			return false;
		}
		
		// Get the video decoder
		m_codec = avcodec_find_decoder(m_codecCtx->codec_id);
		if (NULL == m_codec)
		{
			std::cerr << "Movie_video::Initialize() - could not find any video decoder for this video format" << std::endl;
			Close();
			return false;
		}
		
		// Load the video codec
		err = avcodec_open(m_codecCtx, m_codec);
		if (err < 0)
		{
			std::cerr << "Movie_video::Initialize() - unable to load the video decoder for this video format" << std::endl;
			Close();
			return false;
		}
		
		
		// Create the frame buffers
		m_rawFrame = avcodec_alloc_frame();
		m_RGBAFrame = avcodec_alloc_frame();
		if (!m_rawFrame || !m_RGBAFrame)
		{
			std::cerr << "Movie_video::Initialize() - allocation error" << std::endl;
			Close();
			return false;
		}
		
		// Get the video size
		m_size = sf::Vector2i(m_codecCtx->width, m_codecCtx->height);
		
		// Disable smoothing when the video is not scaled
		sf::Vector2f sc = m_parent.GetScale();
		
		if (fabs(sc.x - 1.f) < 0.00001 &&
			fabs(sc.y - 1.f) < 0.00001)
		{
			m_smooth = false;
		}
		else
		{
			m_smooth = true;
		}
		
		// Get the picture size and create the buffer
		// NB: I don't understand why this is needed, but the video
		// won't display without this, and I wasn't able to contact
		// the author of these lines
		int pictSize = avpicture_get_size(PIX_FMT_RGBA, m_size.x, m_size.y);
		sf::Uint8 *videoBuffer = (sf::Uint8 *)av_malloc(pictSize * sizeof(sf::Uint8));
		if (!videoBuffer)
		{
			std::cerr << "Movie_video::Initialize() - allocation error" << std::endl;
			Close();
			return false;
		}
		
		// Fill the picture buffer with the frame data
		avpicture_fill((AVPicture *)m_RGBAFrame, videoBuffer, PIX_FMT_RGBA,
					   m_size.x, m_size.y);
		av_free(videoBuffer);
		
		m_pictureBuffer = (sf::Uint8 *)av_malloc(sizeof(sf::Uint8) * m_size.x * m_size.y * 4);
		if (!m_pictureBuffer)
		{
			std::cerr << "Movie_video::Initialize() - allocation error" << std::endl;
			Close();
			return false;
		}
		
		// Setup the image scaler/converter
		m_swsCtx = sws_getContext(m_size.x, m_size.y,
								  m_codecCtx->pix_fmt,
								  m_size.x, m_size.y,
								  PIX_FMT_RGBA,
								  SWS_BILINEAR, NULL, NULL, NULL);
		if (!m_swsCtx)
		{
			std::cerr << "Movie_video::Initialize() - error with sws_getContext()" << std::endl;
			Close();
			return false;
		}
		
		
		
		// Setup the SFML stuff
		for (unsigned i = 0; i < MAXIMUM_QUEUE_LENGTH; i++)
		{
			sf::Texture *item = new sf::Texture();
			if (!item->Create(m_size.x, m_size.y))
			{
				std::cerr << "Movie_video::Initialize() - allocation error" << std::endl;
				Close();
				return false;
			}
			
			item->SetSmooth(m_smooth);
			m_freeTexturesQueue.push(item);
		}
		
		// Get the frame time we need for this video
		AVRational r = m_parent.GetAVFormatContext()->streams[m_streamID]->avg_frame_rate;
        AVRational r2 = m_parent.GetAVFormatContext()->streams[m_streamID]->r_frame_rate;
		if ((!r.num || !r.den) &&
            (!r2.num || !r2.den))
        {
			if (Movie::UsesDebugMessages())
				std::cerr << "Movie_video::Initialize() - unable to get the video frame rate. Using standard NTSC frame rate : 29.97 fps." << std::endl;
            m_wantedFrameTime = 1.f / NTSC_FRAMERATE;
        }
        else
        {
            if (r.num && r.den)
			{
                m_wantedFrameTime = 1.f/((float)r.num / r.den);
				
				if (Movie::UsesDebugMessages())
					std::cerr << "Using video framerate : " << ((float)r.num / r.den) << std::endl;
			}
            else
			{
                m_wantedFrameTime = 1.f/((float)r2.num / r2.den);
				
				if (Movie::UsesDebugMessages())
					std::cerr << "Using video framerate : " << ((float)r2.num / r2.den) << std::endl;
			}
        }
		
		if (Movie::UsesDebugMessages())
			std::cerr << "Wanted frame time is " << m_wantedFrameTime << std::endl;
		
		// Get the video duration
		if (m_parent.GetAVFormatContext()->duration != AV_NOPTS_VALUE)
		{
            long secs, us;
            secs = m_parent.GetAVFormatContext()->duration / AV_TIME_BASE;
            us = m_parent.GetAVFormatContext()->duration % AV_TIME_BASE;
			m_parent.SetDuration((secs + (float)us / AV_TIME_BASE) * 1000);
		}
		else
		{
			if (Movie::UsesDebugMessages())
				std::cerr << "Movie_video::Initialize() - warning: unable to retrieve the video duration" << std::endl;
		}
		
		return true;
	}
	
	
	void Movie_video::Close(void)
	{
		// Close the video stuff
		if (m_codecCtx)
			avcodec_close(m_codecCtx), m_codecCtx = NULL;
		
		m_codec = NULL;
		
		if (m_rawFrame)
			av_free(m_rawFrame), m_rawFrame = NULL;
		if (m_RGBAFrame)
			av_free(m_RGBAFrame), m_RGBAFrame = NULL;
		
		// Free the remaining accumulated packets
		while (m_packetList.size()) {
			PopFrame();
		}
		
		if (m_swsCtx)
			sws_freeContext(m_swsCtx), m_swsCtx = NULL;
		
		m_streamID = -1;
		
		if (m_pictureBuffer)
			av_free(m_pictureBuffer), m_pictureBuffer = NULL;
		
		m_isLate = false;
		m_wantedFrameTime = 0.f;
		m_displayedFrameCount = 0;
		m_decodingTime = 0;
		m_size = sf::Vector2i(0, 0);
	}
	
	
#pragma mark -
#pragma mark Movie playback controls
	////////////////////////////////////////////////////////////////////////////
	/// Movie playback controls
	////////////////////////////////////////////////////////////////////////////
	void Movie_video::Play(void)
	{
		// Start threads
		m_running = 1;
		m_canDecodeOneMoreTexture.Restore();
		m_hasImageReadyForDisplay.Restore();
		m_running.Restore();
		
		if (m_parent.GetStatus() != Movie::Paused)
		{
			m_updateThread.Launch();
			m_decodeThread.Launch();
		}
	}
	
	void Movie_video::Pause(void)
	{
		// Stop thread
        /*m_runThread = false;
		m_backImageReady.invalidate();
		m_updateThread.Wait();
		m_decodeThread.Wait();*/
		m_running = 0;
	}
	
	void Movie_video::Stop(void)
	{
		// Stop threads
		//if (m_running.value() == 0)
		{
			m_canDecodeOneMoreTexture.Invalidate();
			m_hasImageReadyForDisplay.Invalidate();
			m_running.Invalidate();
			m_updateThread.Wait();
			m_decodeThread.Wait();
		}
		
		m_displayedFrameCount = 0;
		m_isStarving = false;
		
		// Go back to the beginning of the movie
		if (av_seek_frame(m_parent.GetAVFormatContext(), m_streamID, 0, AVSEEK_FLAG_BACKWARD) < 0)
		{
			std::cerr << "Movie_video::Stop() - av_seek_frame() error" << std::endl;
		}
		
		while (m_packetList.size()) {
			PopFrame();
		}
	}
	
	
	void Movie_video::SetPlayingOffset(sf::Uint32 time)
	{
		Stop();
		
		// TODO: does not work yet
		AVRational tb = m_parent.GetAVFormatContext()->streams[m_streamID]->time_base;
		float ftb = (float)tb.num / tb.den;
		int64_t avTime = time * ftb;
		int res = av_seek_frame(m_parent.GetAVFormatContext(), m_streamID, avTime, AVSEEK_FLAG_BACKWARD);
		
		if (res < 0)
		{
			std::cerr << "Movie_video::SeekToTime() - av_seek_frame() failed" << std::endl;
		}
		else
		{
			Play();
			m_displayedFrameCount = time / m_wantedFrameTime;
		}
	}
	
	
#pragma mark -
#pragma mark sf::Drawable rendering
	////////////////////////////////////////////////////////////////////////////
	/// sf::Drawable rendering
	////////////////////////////////////////////////////////////////////////////
	void Movie_video::Render(sf::RenderTarget& Target) const
	{
		// We're on the rendering thread! Do the texture upload stuff
		
		//while ();
		
		SHOW_LOCKTIME( m_texturesQueueMutex.Lock() ); // 2% on Windows
		glFlush(); // make sure the texture has been updated from the context sharing space
		Target.Draw(m_sprite); // 38% on Windows
		m_texturesQueueMutex.Unlock();
		
		// Allow thread switching
		sf::Sleep(0);
	}
	
	
	void Movie_video::DisplayNextImage(bool unconditionned)
	{
		// Make sure the back image is ready for swaping
		if (unconditionned || m_hasImageReadyForDisplay.WaitAndLock(1))
		{
			// Make sure we don't swap while using front/backImage()
			SHOW_LOCKTIME(m_texturesQueueMutex.Lock());
			PopTexture_unlocked(); // recycle texture
			sf::Texture& tex = FrontTexture_unlocked();
			
			m_sprite.SetTexture(tex); // take the new front one
			
			m_texturesQueueMutex.Unlock();
			
			// m_hasImageReadyForDisplay isn't locked when we come here in an unconditionned (loading forced) state
			if (!unconditionned)
				m_hasImageReadyForDisplay.Unlock(ReadyTexturesQueueLength() > 1 ? 1 : 0);
			
			m_canDecodeOneMoreTexture = 1; // We just popped one, so there is at least one free place!
		}
	}
	
	
#pragma mark -
#pragma mark Get some information
	////////////////////////////////////////////////////////////////////////////
	/// Get some information
	////////////////////////////////////////////////////////////////////////////
	int Movie_video::GetStreamID(void) const
	{
		return m_streamID;
	}
	
	
	const sf::Vector2i& Movie_video::GetSize(void) const
	{
		return m_size;
	}
	
	
	float Movie_video::GetWantedFrameTime(void) const
	{
		return m_wantedFrameTime;
	}
	
	
	sf::Image Movie_video::GetImageCopy(void) const
	{
		sf::Lock l(m_texturesQueueMutex);
		return FrontTexture().CopyToImage();
	}
	
	
#pragma mark -
#pragma mark Video threads
	////////////////////////////////////////////////////////////////////////////
	/// Video threads
	////////////////////////////////////////////////////////////////////////////
	void Movie_video::UpdateThreadCallback(void)
	{
		// Run while m_running is valid, and wait when we're pausing
		while (m_running.WaitAndLock(1, Condition::AutoUnlock))
		{
			sf::Uint32 waitTime;
			m_isLate = IsLate(waitTime);
			
			// Wait for the right time if needed
			if (waitTime)
			{
				if (waitTime > 1000)
					std::cout << "waiting for weird time: " << waitTime << std::endl;
				sf::Sleep(waitTime);
			}
			
			// Show next!
			DisplayNextImage();
		}
	}
	
	void Movie_video::DecodeThreadCallback(void)
	{	
		while (m_running.WaitAndLock(1, Condition::AutoUnlock) &&
			   m_canDecodeOneMoreTexture.WaitAndLock(1))
		{
			sf::Uint32 waitTime;
			m_isLate = IsLate(waitTime);
			
			if (m_isLate)
			{
				unsigned lateFrames = LateFramesCount();
				
				if (Movie::UsesDebugMessages())
					std::cerr << "Movie_video::DecodeThreadCallback() - we are late and skipping " << lateFrames << " frames\n";
				
				while (lateFrames--)
					LoadNextImage(SKIP_TEXTURE_UPLOAD);
				
				m_isLate = false;
			}
			
			// Allow loading some more textures depending on whether
			// last loading succeeded and whether there is still some
			// free space
			if (LoadNextImage())
			{
				m_canDecodeOneMoreTexture.Unlock(CanStoreMoreTextures());
				
				m_texturesQueueMutex.Lock();
				if (ReadyTexturesQueueLength_unlocked() > 1)
					m_hasImageReadyForDisplay = 1;
				m_texturesQueueMutex.Unlock();
			}
			else
				m_canDecodeOneMoreTexture.Unlock(1);
			
			// If we're starving, tell it to the parent
			if (m_isStarving)
			{
				m_parent.Starvation();
			}
		}
	}
	
	
#pragma mark -
#pragma mark Get some states
	////////////////////////////////////////////////////////////////////////////
	/// Get some states
	////////////////////////////////////////////////////////////////////////////
	bool Movie_video::IsLate(void)
	{
		sf::Uint32 waitTime;
		return IsLate(waitTime);
	}
	
	
	bool Movie_video::IsLate(sf::Uint32& availableSleepTime)
	{
		bool isLate = false;
		availableSleepTime = 0;
		// Get the 'real' time from the start of the video
		// and the progress of the video
		
		// Here is the real time elapsed since we started to play the video
		sf::Uint32 realTime = m_parent.GetPlayingOffset();
		
		// Here is the time we're at in the video
		// Note: m_wantedFrameTime is kept as float (seconds) for accuracy
		sf::Uint32 readyTextures = ReadyTexturesQueueLength();
		sf::Uint32 movieTime = (m_displayedFrameCount - readyTextures) * m_wantedFrameTime * 1000;
		//sf::Uint32 movieTime = m_loadedFrameCount * m_wantedFrameTime * 1000;
		
		if ((int)m_displayedFrameCount - (int)readyTextures > 0 && movieTime > realTime + m_wantedFrameTime * 1000)
		{
			isLate = false;
			
			// Added a check to prevent from waiting if we've stopped
			// the movie playback (and thus waiting for abnormal periods of time)
			if (m_parent.GetStatus() == Movie::Playing)
				availableSleepTime = (movieTime - realTime - m_wantedFrameTime * 1000);
		}
		else
		{
			// don't skip a frame if we just have one frame late,
			// it may be because of a occasional slowdown
			if (movieTime < realTime - m_wantedFrameTime * ALLOWED_LATE_FRAMES_COUNT * 1000 && m_decodingTime)
			{
				isLate = true;
				availableSleepTime = 0;
				
				//if (Movie::UsesDebugMessages())
				//	std::cerr << "Movie_video::Run() - warning: skipping frame because we are late by " << (realTime - movieTime) << "ms (movie playing offset is " << realTime << "ms)" << std::endl;
			}
			else if (movieTime < realTime - m_wantedFrameTime && m_decodingTime)
			{
				isLate = false;
				availableSleepTime = 0;
				//if (Movie::UsesDebugMessages())
				//	std::cerr << "Movie_video::Run() - warning: movie playback is late by " << (realTime - movieTime) << "ms (movie playing offset is " << realTime << "ms) but we're not skipping any frame since we're not 'too' late" << std::endl;
			}
		}
		
		if (availableSleepTime > 1000)
		{
			std::cout << "huhu ... " << std::endl;
		}
		
		return isLate;
	}
	
	
	unsigned Movie_video::LateFramesCount(void)
	{
		unsigned lateFrames = 0;
		sf::Uint32 realTime = m_parent.GetPlayingOffset();
		sf::Uint32 movieTime = (m_displayedFrameCount - ReadyTexturesQueueLength()) * m_wantedFrameTime * 1000;
		
		sf::Uint32 lateTime = 0;
		
		if (movieTime < realTime - m_wantedFrameTime * ALLOWED_LATE_FRAMES_COUNT * 1000 && m_decodingTime)
		{
			lateTime = realTime - movieTime;
			lateFrames = lateTime / (m_wantedFrameTime * 1000);
		}
		
		return lateFrames;
	}
	
	
	bool Movie_video::IsStarving(void)
	{
		return m_isStarving;
	}
	
#pragma mark -
#pragma mark Image loading
	////////////////////////////////////////////////////////////////////////////
	/// Image loading
	////////////////////////////////////////////////////////////////////////////
	bool Movie_video::PreLoad(void)
	{
		bool flag = false;
		int tries = 0;
		while (false == LoadNextImage() && tries++ < 10); // First frame always gives "frame not decoded"
		
		// Abort if we can't load frames
		if (tries != 10)
		{
			while (CanStoreMoreTextures() && LoadNextImage());
			DisplayNextImage(true);
			flag = true;
		}
		
		return flag;
	}
	
	
	bool Movie_video::LoadNextImage(bool skipUpload)
	{
		bool flag = false;
		m_timer.Reset();
		
		// If our video packet list is empty, load one more video frame
		if (!HasPendingFrame())
		{
			if (!ReadAndPushFrame())
			{
			    // Stop if there is no more data to read
				if (Movie::UsesDebugMessages())
					std::cerr << "Movie_video::Update() - end of video stream reached." << std::endl;
				
				m_isStarving = true;
			}
			else
				flag = DecodeFrontFrame(skipUpload);
		}
		else
		{
			flag = DecodeFrontFrame(skipUpload);
		}
		m_decodingTime = m_timer.GetElapsedTime();
		
		return flag;
	}
	
	
	bool Movie_video::DecodeFrontFrame(bool skipUpload)
	{
		// whole function takes about 50% CPU with 2048x872 definition on Mac OS X
		// 50% (one full core) on Windows
		int didDecodeFrame = 0;
		bool flag = false;
		
		assert(CanStoreMoreTextures());
		
		// Stop here if there is no frame to decode
		if (!HasPendingFrame())
		{
			if (Movie::UsesDebugMessages())
				std::cerr << "Movie_video::DecodeFrontFrame() - no frame currently available for decoding" << std::endl;
			return flag;
		}
		
		// Get the front frame and decode it
		AVPacket *videoPacket = FrontFrame();
		int res;
		res = avcodec_decode_video2(m_codecCtx, m_rawFrame, &didDecodeFrame,
									videoPacket); // 20% (40% of total function) on macosx; 18.3% (36% of total) on windows
		
		if (!skipUpload)
		{
			if (res < 0)
			{
				std::cerr << "Movie_video::DecodeFrontFrame() - an error occured while decoding the video frame (code "
				<< res << ")" << std::endl;
			}
			else
			{
				if (didDecodeFrame)
				{
					// Convert the frame to RGBA
					// FIXME: crash here (in the function sws_getDefaultFilter()
					// called by sws_scale()) when GuardMalloc is enabled, but
					// I don't know whether this is because of the 16 bytes boundaries
					// alignement constraint
					sws_scale(m_swsCtx,
							  m_rawFrame->data, m_rawFrame->linesize,
							  0, m_codecCtx->height,
							  m_RGBAFrame->data, m_RGBAFrame->linesize); // 6.3% on windows (12% of total)
					
					// Load the data in the texture
					sf::Texture *freeTexture = TakeFreeTexture();
					// 10.1% (25% of total function) on macosx ; 18.4% (37% of total) on windows (old numbers)
					freeTexture->Update((sf::Uint8*)m_RGBAFrame->data[0]);
					//freeTexture->Update((sf::Uint8*)m_rawFrame->data[0]);
					glFlush(); // Send the texture to all of the other shared contexts
					PushTexture(freeTexture); // put the texture in the readdy-to-be-displayed list
					
					// Image loaded, reset condition state
					flag = true;
				}
				else
				{
					if (m_parent.Movie::UsesDebugMessages())
						std::cerr << "Movie_video::DecodeFrontFrame() - frame not decoded" << std::endl;
				}
			}
		}
		
		PopFrame();
		m_displayedFrameCount++;
		
		return flag;
	}
	
	
#pragma mark -
#pragma mark Raw frames (non-decoded) storing
	////////////////////////////////////////////////////////////////////////////
	/// Raw frames (non-decoded) storing
	////////////////////////////////////////////////////////////////////////////
	void Movie_video::PushFrame(AVPacket *pkt)
	{
		SHOW_LOCKTIME(m_packetListMutex.Lock());
		m_packetList.push(pkt);
		m_packetListMutex.Unlock();
	}
	
	bool Movie_video::ReadAndPushFrame(void)
	{
		while (!HasPendingFrame() &&
			   m_parent.ReadFrameAndQueue());
		
		return HasPendingFrame();
	}
	
	void Movie_video::PopFrame(void)
	{
		SHOW_LOCKTIME(m_packetListMutex.Lock());
		
		if (!m_packetList.empty())
		{
			AVPacket *pkt = m_packetList.front();
			m_packetList.pop();
			av_free_packet(pkt);
			av_free(pkt);
		}
		
		m_packetListMutex.Unlock();
	}
	
	AVPacket *Movie_video::FrontFrame(void)
	{
		AVPacket *pkt = NULL;
		assert(!m_packetList.empty());
		
		SHOW_LOCKTIME(m_packetListMutex.Lock());
		pkt = m_packetList.front();
		m_packetListMutex.Unlock();
		
		return pkt;
	}
	
	
	bool Movie_video::HasPendingFrame(void)
	{
		return (m_packetList.size() > 0);
	}
	
	
#pragma mark -
#pragma mark Textures storing
	////////////////////////////////////////////////////////////////////////////
	/// Textures storing
	////////////////////////////////////////////////////////////////////////////
	void Movie_video::PushTexture(sf::Texture *tex)
	{
		assert(CanStoreMoreTextures());
		
		SHOW_LOCKTIME(m_texturesQueueMutex.Lock());
		m_readyTexturesQueue.push(tex);
		m_texturesQueueMutex.Unlock();
	}
	
	
	void Movie_video::PopTexture(void)
	{
		SHOW_LOCKTIME(m_texturesQueueMutex.Lock());
		SHOW_LOCKTIME(m_freeTexturesQueueMutex.Lock());
		PopTexture_unlocked();
		m_freeTexturesQueueMutex.Unlock();
		m_texturesQueueMutex.Unlock();
	}
	
	void Movie_video::PopTexture_unlocked(void)
	{
		assert(ReadyTexturesQueueLength() > 0);
		
		m_freeTexturesQueue.push(m_readyTexturesQueue.front());
		m_readyTexturesQueue.pop();
	}
	
	sf::Texture& Movie_video::FrontTexture(void)
	{
		sf::Texture *tex = NULL;
		
		SHOW_LOCKTIME(m_texturesQueueMutex.Lock());
		tex = &FrontTexture_unlocked();
		m_texturesQueueMutex.Unlock();
		
		return *tex;
	}
	
	sf::Texture& Movie_video::FrontTexture_unlocked(void)
	{
		assert(ReadyTexturesQueueLength() > 0);
		
		return *m_readyTexturesQueue.front();
	}
	
	
	const sf::Texture& Movie_video::FrontTexture(void) const
	{
		sf::Texture *tex = NULL;
		assert(ReadyTexturesQueueLength() > 0);
		
		SHOW_LOCKTIME(m_texturesQueueMutex.Lock());
		tex = m_readyTexturesQueue.front();
		m_texturesQueueMutex.Unlock();
		
		return *tex;
	}
	
	
	unsigned Movie_video::ReadyTexturesQueueLength(void) const
	{
		unsigned res = 0;
		
		SHOW_LOCKTIME(m_texturesQueueMutex.Lock());
		res = m_readyTexturesQueue.size();
		m_texturesQueueMutex.Unlock();
		
		return res;
	}
	
	
	unsigned Movie_video::ReadyTexturesQueueLength_unlocked(void) const
	{
		unsigned res = 0;
		
		res = m_readyTexturesQueue.size();
		
		return res;
	}
	
	
	bool Movie_video::CanStoreMoreTextures(void) const
	{
		bool res = false;
		
		SHOW_LOCKTIME(m_texturesQueueMutex.Lock());
		res = (m_readyTexturesQueue.size() < MAXIMUM_QUEUE_LENGTH);
		m_texturesQueueMutex.Unlock();
		
		return res;
	}
	
	
	sf::Texture *Movie_video::TakeFreeTexture(void)
	{
		sf::Texture *tex = NULL;
		
		SHOW_LOCKTIME(m_freeTexturesQueueMutex.Lock());
		if (m_freeTexturesQueue.size() > 0)
		{
			tex = m_freeTexturesQueue.front();
			m_freeTexturesQueue.pop();
		}
		m_freeTexturesQueueMutex.Unlock();
		
		return tex;
	}
	
} // namespace sfe

