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
#include <SFML/Config.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <cassert>
#include "utils.h"

#define NTSC_FRAMERATE 29.97f


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
	m_packetListMutex(),
	
	// Threads
	m_updateThread(&Movie_video::Update, this),	// Does swaping and time sync
	m_decodeThread(&Movie_video::Decode, this),	// Does video decoding
	m_running(),
	
	// Image swaping
	m_imageSwapMutex(),
	m_backImageReady(),
	m_imageIndex(0),
	m_image1(),
	m_image2(),
	
	// Miscellaneous parameters
	m_isLate(false),
	m_isStarving(false),
	m_sprite(),
	m_wantedFrameTime(0.f),
	m_displayedFrameCount(0),
	m_decodingTime(0),
	m_timer(),
	m_runThread(false),
	m_size(0, 0)
	{
		
	}
	
	Movie_video::~Movie_video(void)
	{
	}
	
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
		
		// Get the picture size and create the buffer
		// NB: I don't understand why this is needed, but the video
		// won't display without this, and I wasn't able to contact
		// the author of these lines
		int pictSize = avpicture_get_size(PIX_FMT_RGBA,
										  m_codecCtx->width,
										  m_codecCtx->height);
		sf::Uint8 *videoBuffer = (sf::Uint8 *)av_malloc(pictSize * sizeof(sf::Uint8));
		if (!videoBuffer)
		{
			std::cerr << "Movie_video::Initialize() - allocation error" << std::endl;
			Close();
			return false;
		}
		
		// Fill the picture buffer with the frame data
		avpicture_fill((AVPicture *)m_RGBAFrame, videoBuffer, PIX_FMT_RGBA,
					   m_codecCtx->width, m_codecCtx->height);
		av_free(videoBuffer);
		
		m_pictureBuffer = (sf::Uint8 *)av_malloc(sizeof(sf::Uint8) * m_codecCtx->width * m_codecCtx->height * 4);
		if (!m_pictureBuffer)
		{
			std::cerr << "Movie_video::Initialize() - allocation error" << std::endl;
			Close();
			return false;
		}
		
		// Setup the image scaler/converter
		m_swsCtx = sws_getContext(m_codecCtx->width, m_codecCtx->height,
								  m_codecCtx->pix_fmt,
								  m_codecCtx->width, m_codecCtx->height,
								  PIX_FMT_RGBA,
								  SWS_BICUBIC, NULL, NULL, NULL);
		if (!m_swsCtx)
		{
			std::cerr << "Movie_video::Initialize() - error with sws_getContext()" << std::endl;
			Close();
			return false;
		}
		
		// Setup the SFML stuff
		if (!m_image1.Create(m_codecCtx->width, m_codecCtx->height) || !m_image2.Create(m_codecCtx->width, m_codecCtx->height))
		{
			std::cerr << "Movie_video::Initialize() - allocation error" << std::endl;
			Close();
			return false;
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
					std::cerr << "Using video framerate : " << ((float)r.num / r.den) << std::endl;
			}
        }
		
		if (Movie::UsesDebugMessages())
			std::cerr << "Wanted frame time is " << m_wantedFrameTime << std::endl;
		
		// Get the video size
		m_size = sf::Vector2i(m_codecCtx->width, m_codecCtx->height);
		
		// Get the video duration
		if (m_parent.GetAVFormatContext()->duration != AV_NOPTS_VALUE)
		{
            long secs, us;
            secs = m_parent.GetAVFormatContext()->duration / AV_TIME_BASE;
            us = m_parent.GetAVFormatContext()->duration % AV_TIME_BASE;
			m_parent.SetDuration((float)secs + (float)us / AV_TIME_BASE);
		}
		else
		{
			if (Movie::UsesDebugMessages())
				std::cerr << "Movie_video::Initialize() - warning: unable to retrieve the video duration" << std::endl;
		}
		
		return true;
	}
	
	void Movie_video::Play(void)
	{
		// Disable smoothing when the video is not scaled
		sf::Vector2f sc = m_parent.GetScale();
		
		if (fabs(sc.x - 1.f) < 0.00001 &&
			fabs(sc.y - 1.f) < 0.00001)
		{
			m_image1.SetSmooth(false);
			m_image2.SetSmooth(false);
		}
		else
		{
			m_image1.SetSmooth(true);
			m_image2.SetSmooth(true);
		}
		
		// Start threads
		m_runThread = true;
		m_running = 1;
		m_backImageReady.restore();
		m_running.restore();
		
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
		if (m_runThread)
		{
			//m_updateThreadWatcher.Terminate(); //Terminate because if stopping isn't caused by reaching the Eof, it will try to call Stop a second time
            m_runThread = false;
			m_backImageReady.invalidate();
			m_running.invalidate();
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
		m_decodingTime = 0.f;
		m_runThread = false;
		m_size = sf::Vector2i(0, 0);
	}
	
	void Movie_video::Render(sf::RenderTarget& Target) const
	{
		sf::Lock l(m_imageSwapMutex); // 2% on Windows
		Target.Draw(m_sprite); // 38% on Windows
		
		// Allow thread switching
		sf::Sleep(0);
	}
	
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
		sf::Lock l(m_imageSwapMutex);
		return FrontImage();
	}
	
	void Movie_video::Update(void)
	{
		while (m_runThread && m_running.waitForValueAndRetain(1, Condition::Autorelease))
		{
			float waitTime = UpdateLateState();
			
			if (!m_isLate)
				sf::Sleep(waitTime);
			
			SwapImages();
		}
	}
	
	void Movie_video::Decode(void)
	{
		while (m_runThread &&
			   m_running.waitForValueAndRetain(1, Condition::Autorelease) &&
			   m_backImageReady.waitForValueAndRetain(0))
		{
			UpdateLateState();
			
			if (LoadNextImage())
				m_backImageReady.release(1);
			else
				m_backImageReady.release(0);
			
			if (m_isStarving)
			{
				m_parent.Starvation();
			}
		}
	}
	
	float Movie_video::UpdateLateState(void)
	{
		float waitTime = 0.f;
		// Get the 'real' time from the start of the video
		// and the progress of the video
		
		// Here is the real time elapsed since we started to play the video
		float realTime = m_parent.GetPlayingOffset();
		
		// Here is the time we're at in the video
		float movieTime = m_displayedFrameCount * m_wantedFrameTime;
		
		if (movieTime > realTime + m_wantedFrameTime)
		{
			m_isLate = false;
			
			// Added a check to prevent from waiting if we've stopped
			// the movie playback (and thus waiting for abnormal periods of time)
			if (m_parent.GetStatus() == Movie::Playing)
				waitTime = (movieTime - realTime - m_wantedFrameTime);
		}
		else
		{
			// don't skip a frame if we just have one frame late,
			// it may be because of a occasional slowdown
			if (movieTime < realTime - m_wantedFrameTime * 3 && m_decodingTime)
			{
				m_isLate = true;
				
				if (Movie::UsesDebugMessages())
					std::cerr << "Movie_video::Run() - warning: skipping frame because we are late by " << (realTime - movieTime) << "s (movie playing offset is " << realTime << "s)" << std::endl;
			}
			else if (movieTime < realTime - m_wantedFrameTime && m_decodingTime)
			{
				if (Movie::UsesDebugMessages())
					std::cerr << "Movie_video::Run() - warning: movie playback is late by " << (realTime - movieTime) << "s (movie playing offset is " << realTime << "s) but we're not skipping any frame since we're not 'too' late" << std::endl;
			}
		}
		
		return waitTime;
	}
	
	
	bool Movie_video::IsStarving(void)
	{
		return m_isStarving;
	}
	
	
	void Movie_video::SetPlayingOffset(float time)
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
	
	void Movie_video::SwapImages(bool unconditionned)
	{
		// Make sure the back image is ready for swaping
		if (unconditionned || m_backImageReady.waitForValueAndRetain(1))
		{
			// Make sure we don't swap while using front/backImage()
			m_imageSwapMutex.Lock();
			m_imageIndex = (m_imageIndex + 1) % 2;
			m_sprite.SetImage((m_imageIndex == 0) ? m_image1 : m_image2);
			m_imageSwapMutex.Unlock();
			
			// Update condition
			if (!unconditionned)
				m_backImageReady.release(0);
		}
	}
	
	sf::Image& Movie_video::FrontImage(void)
	{
		sf::Lock l(m_imageSwapMutex);
		return (m_imageIndex == 0) ? m_image1 : m_image2;
	}
	
	const sf::Image& Movie_video::FrontImage(void) const
	{
		return (m_imageIndex == 0) ? m_image1 : m_image2;
	}
	
	sf::Image& Movie_video::BackImage(void)
	{
		return (m_imageIndex == 0) ? m_image2 : m_image1;
	}
	
	
	bool Movie_video::PreLoad(void)
	{
		int counter = 0;
		bool res = false;
		while (false == (res = LoadNextImage()) && counter < 10) // First frame always gives "frame not decoded"
			counter++;
		
		// Abort if we can't load frames
		if (counter == 10)
			return false;
		
		SwapImages(true);
		LoadNextImage();
		m_backImageReady = 1;
	}
	
	
	bool Movie_video::LoadNextImage(void)
	{
		bool flag = false;
		m_timer.Reset();
		
		// If our video packet list is empty, load one more video frame
		if (!HasPendingDecodableData())
		{
			if (!ReadFrame())
			{
			    // Stop if there is no more data to read
				if (Movie::UsesDebugMessages())
					std::cerr << "Movie_video::Update() - end of video stream reached." << std::endl;
				
				m_isStarving = true;
			}
			else
				flag = DecodeFrontFrame();
		}
		else
		{
			flag = DecodeFrontFrame();
		}
		m_decodingTime = m_timer.GetElapsedTime();
		return flag;
	}
	
	bool Movie_video::ReadFrame(void)
	{
		while (!HasPendingDecodableData() &&
			   m_parent.ReadFrameAndQueue());
		
		return HasPendingDecodableData();
	}
	
	bool Movie_video::HasPendingDecodableData(void)
	{
		return (m_packetList.size() > 0);
	}
	
	bool Movie_video::DecodeFrontFrame(void)
	{
		// whole function takes about 50% CPU with 2048x872 definition on Mac OS X
		// 50% (one full core) on Windows
		int didDecodeFrame = 0;
		bool flag = false;
		
		// Stop here if there is no frame to decode
		if (!HasPendingDecodableData())
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
		
		if (!m_isLate)
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
					
					
					// Load the data in the sf::Image
					m_imageSwapMutex.Lock();
					
					// 10.1% (25% of total function) on macosx ; 18.4% (37% of total) on windows
					BackImage().LoadFromPixels(m_codecCtx->width, m_codecCtx->height, (sf::Uint8*)m_RGBAFrame->data[0]);
					
					// 7.7% (19% of total function) on macosx ; 6.13% (12% of total function) on windows
					m_imageSwapMutex.Unlock();
					
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
		//m_isLate = false;
		
		return flag;
	}
	
	void Movie_video::PushFrame(AVPacket *pkt)
	{
		sf::Lock l(m_packetListMutex);
		m_packetList.push(pkt);
	}
	
	void Movie_video::PopFrame(void)
	{
		sf::Lock l(m_packetListMutex);
		
		if (!m_packetList.empty())
		{
			AVPacket *pkt = m_packetList.front();
			m_packetList.pop();
			av_free_packet(pkt);
			av_free(pkt);
		}
	}
	
	AVPacket *Movie_video::FrontFrame(void)
	{
		assert(!m_packetList.empty());
		
		sf::Lock l(m_packetListMutex);
		return m_packetList.front();
	}
	
	/*void Movie_video::WatchThread(void)
	{
	    while(m_runThread)
	    {
	        sf::Sleep(0.1);
	    }
		
	    //m_parent.Stop();
	}*/
	
} // namespace sfe

