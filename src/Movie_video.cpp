/*
 *  Movie_video.cpp
 *  SFE (SFML Extension) project
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



#include "Movie_video.h"
#include "Movie.h"

#include "Movie_audio.h"
#include <SFML/Config.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <cassert>

#define NTSC_FRAMERATE 29.97f

namespace sfe {
	
	Movie_video::Movie_video(Movie& parent) :
	m_parent(parent),
	
	// Image and decoding stuff
	m_codecCtx(NULL),
	m_codec(NULL),
	m_rawFrame(NULL),
	m_backRGBAFrame(NULL),
	m_frontRGBAFrame(NULL),
	m_streamID(-1),
	m_pictureBuffer(NULL), // Buffer used to convert image from pixel matrix to simple array
	m_swsCtx(NULL),
	
	// Packets' queueing stuff
	m_packetList(),
	m_packetListMutex(),
	
	// Decoding thread
	m_decodeThread(&Movie_video::decode, this),	// Does video decoding
	m_running(),
	
	// Image swaping
	m_imageSwapMutex(),
	m_backImageReady(),
	m_imageIndex(0),
	m_tex(),
	
	// Miscellaneous parameters
	m_isStarving(false),
	m_sprite(),
	m_wantedFrameTime(sf::Time::Zero),
	m_displayedFrameCount(0),
	m_decodingTime(sf::Time::Zero),
	m_timer(),
	m_runThread(false),
	m_size(0, 0)
	{
		
	}
	
	Movie_video::~Movie_video(void)
	{
	}
	
	bool Movie_video::initialize(void)
	{
		int err;
		
		// Find the video stream among the differents streams
		for (int i = 0; -1 == m_streamID && i < m_parent.getAVFormatContext()->nb_streams; i++)
		{
			if (m_parent.getAVFormatContext()->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
				m_streamID = i;
		}
		
		// If no video stream found...
		if (-1 == m_streamID)
		{
			close();
			return false;
		}
		
		// Get the video codec
		m_codecCtx = m_parent.getAVFormatContext()->streams[m_streamID]->codec;
		if (!m_codecCtx)
		{
			std::cerr << "Movie_video::Initialize() - unable to get the video codec context" << std::endl;
			close();
			return false;
		}
		
		// Get the video decoder
		m_codec = avcodec_find_decoder(m_codecCtx->codec_id);
		if (NULL == m_codec)
		{
			std::cerr << "Movie_video::Initialize() - could not find any video decoder for this video format" << std::endl;
			close();
			return false;
		}
		
		// Load the video codec
		err = avcodec_open2(m_codecCtx, m_codec, NULL);
		if (err < 0)
		{
			std::cerr << "Movie_video::Initialize() - unable to load the video decoder for this video format" << std::endl;
			close();
			return false;
		}
		
		
		// Create the frame buffers
		m_rawFrame = avcodec_alloc_frame();
		m_backRGBAFrame = avcodec_alloc_frame();
		m_frontRGBAFrame = avcodec_alloc_frame();
		if (!m_rawFrame || !m_frontRGBAFrame || !m_backRGBAFrame)
		{
			std::cerr << "Movie_video::Initialize() - allocation error" << std::endl;
			close();
			return false;
		}
		
		// Get the video size
		m_size = sf::Vector2i(m_codecCtx->width, m_codecCtx->height);
		
		// Get the picture size and create the buffer
		// NB: I don't understand why this is needed, but the video
		// won't display without this, and I wasn't able to contact
		// the author of these lines
		int pictSize = avpicture_get_size(PIX_FMT_RGBA, m_size.x, m_size.y);
		sf::Uint8 *videoBuffer = (sf::Uint8 *)av_malloc(pictSize * sizeof(sf::Uint8));
		if (!videoBuffer)
		{
			std::cerr << "Movie_video::Initialize() - allocation error" << std::endl;
			close();
			return false;
		}
		
		// Fill the picture buffer with the frame data
		avpicture_fill((AVPicture *)m_frontRGBAFrame, videoBuffer, PIX_FMT_RGBA,
					   m_size.x, m_size.y);
		avpicture_fill((AVPicture *)m_backRGBAFrame, videoBuffer, PIX_FMT_RGBA,
					   m_size.x, m_size.y);
		av_free(videoBuffer);
		
		m_pictureBuffer = (sf::Uint8 *)av_malloc(sizeof(sf::Uint8) * m_size.x * m_size.y * 4);
		if (!m_pictureBuffer)
		{
			std::cerr << "Movie_video::Initialize() - allocation error" << std::endl;
			close();
			return false;
		}
		
		// Setup the image scaler/converter
		m_swsCtx = sws_getContext(m_size.x, m_size.y,
								  m_codecCtx->pix_fmt,
								  m_size.x, m_size.y,
								  PIX_FMT_RGBA,
								  SWS_BICUBIC, NULL, NULL, NULL);
		if (!m_swsCtx)
		{
			std::cerr << "Movie_video::Initialize() - error with sws_getContext()" << std::endl;
			close();
			return false;
		}
		
		
		
		// Setup the SFML stuff
		m_tex.create(m_size.x, m_size.y);
		m_sprite.setTexture(m_tex);
		
		// Get the frame time we need for this video
		AVRational r = m_parent.getAVFormatContext()->streams[m_streamID]->avg_frame_rate;
        AVRational r2 = m_parent.getAVFormatContext()->streams[m_streamID]->r_frame_rate;
		if ((!r.num || !r.den) &&
            (!r2.num || !r2.den))
        {
			if (Movie::usesDebugMessages())
				std::cerr << "Movie_video::Initialize() - unable to get the video frame rate. Using standard NTSC frame rate : 29.97 fps." << std::endl;
            m_wantedFrameTime = sf::seconds(1.f / NTSC_FRAMERATE);
        }
        else
        {
            if (r.num && r.den)
			{
                m_wantedFrameTime = sf::seconds(1.f/((float)r.num / r.den));
				
				if (Movie::usesDebugMessages())
					std::cerr << "Using video framerate : " << ((float)r.num / r.den) << std::endl;
			}
            else
			{
                m_wantedFrameTime = sf::seconds(1.f/((float)r2.num / r2.den));
				
				if (Movie::usesDebugMessages())
					std::cerr << "Using video framerate : " << ((float)r2.num / r2.den) << std::endl;
			}
        }
		
		if (Movie::usesDebugMessages())
			std::cerr << "Wanted frame time is " << m_wantedFrameTime.asMilliseconds() << std::endl;
		
		// Get the video duration
		if (m_parent.getAVFormatContext()->duration != AV_NOPTS_VALUE)
		{
            long secs, us;
            secs = m_parent.getAVFormatContext()->duration / AV_TIME_BASE;
            us = m_parent.getAVFormatContext()->duration % AV_TIME_BASE;
			m_parent.setDuration(sf::seconds(secs + (float)us / AV_TIME_BASE));
		}
		else
		{
			if (Movie::usesDebugMessages())
				std::cerr << "Movie_video::Initialize() - warning: unable to retrieve the video duration" << std::endl;
		}
		
		return true;
	}
	
	void Movie_video::play(void)
	{
		// Disable smoothing when the video is not scaled
		sf::Vector2f sc = m_parent.getScale();
		
		if (fabs(sc.x - 1.f) < 0.00001 &&
			fabs(sc.y - 1.f) < 0.00001)
		{
			m_tex.setSmooth(false);
		}
		else
		{
			m_tex.setSmooth(true);
		}
		
		// Start threads
		m_runThread = true;
		m_running = 1;
		m_backImageReady.restore();
		m_running.restore();
		
		if (m_parent.getStatus() != Movie::Paused)
		{
			m_decodeThread.launch();
		}
	}
	
	void Movie_video::pause(void)
	{
		// Pause threads
		m_running = 0;
	}
	
	void Movie_video::stop(void)
	{
		// Stop threads
		if (m_runThread)
		{
            m_runThread = false;
			m_backImageReady.invalidate();
			m_running.invalidate();
			m_decodeThread.wait();
		}
		
		m_displayedFrameCount = 0;
		m_isStarving = false;
		
		// Go back to the beginning of the movie
		if (av_seek_frame(m_parent.getAVFormatContext(), m_streamID, 0, AVSEEK_FLAG_BACKWARD) < 0)
		{
			std::cerr << "Movie_video::Stop() - av_seek_frame() error" << std::endl;
		}
		
		while (m_packetList.size()) {
			popFrame();
		}
	}
	
	void Movie_video::close(void)
	{
		// Close the video stuff
		if (m_codecCtx)
			avcodec_close(m_codecCtx), m_codecCtx = NULL;
		
		m_codec = NULL;
		
		if (m_rawFrame)
			av_free(m_rawFrame), m_rawFrame = NULL;
		if (m_frontRGBAFrame)
			av_free(m_frontRGBAFrame), m_frontRGBAFrame = NULL;
		if (m_backRGBAFrame)
			av_free(m_backRGBAFrame), m_backRGBAFrame = NULL;
		
		// Free the remaining accumulated packets
		while (m_packetList.size()) {
			popFrame();
		}
		
		if (m_swsCtx)
			sws_freeContext(m_swsCtx), m_swsCtx = NULL;
		
		m_streamID = -1;
		
		if (m_pictureBuffer)
			av_free(m_pictureBuffer), m_pictureBuffer = NULL;
		
		m_wantedFrameTime = sf::Time::Zero;
		m_displayedFrameCount = 0;
		m_decodingTime = sf::Time::Zero;
		m_runThread = false;
		m_size = sf::Vector2i(0, 0);
	}
	
	void Movie_video::draw(sf::RenderTarget& target, sf::RenderStates& states) const
	{
		if (m_backImageReady.value() == 1)
		{
			sf::Time waitTime;
			getLateState(waitTime);
			
			if (waitTime.asMilliseconds() < 10)
			{
				AVFrame *tmpFrame;
				
				// Swap back and front RGBA images buffers
				m_imageSwapMutex.lock();
				tmpFrame = m_frontRGBAFrame;
				m_frontRGBAFrame = m_backRGBAFrame;
				m_backRGBAFrame = tmpFrame;
				m_imageSwapMutex.unlock();
				
				// We update the texture from the front frame while the back frame
				// is being decoded
				m_tex.update((sf::Uint8*)m_frontRGBAFrame->data[0]);
				
				// We unlock the decoding thread after the texture update
				// because otherwise there are artefacts in the displayed
				// image (bug I don't know why because the decoding thread never
				// uses the front frame), even if we could theoretically
				// do this before the Update() call
				m_backImageReady = 0;
			}
		}
		
		/*sf::RectangleShape rec(sf::Vector2f(m_tex.GetWidth() + 2, m_tex.GetHeight() + 2));
		rec.SetPosition(sf::Vector2f(m_sprite.GetPosition().x - 1, m_sprite.GetPosition().y - 1));
		rec.SetOutlineColor(sf::Color::Red);
		rec.SetFillColor(sf::Color::Blue);
		rec.SetOutlineThickness(1.f);
		Target.Draw(rec, states);*/
		
		target.draw(m_sprite, states); // 38% on Windows

		// Allow thread switching
		sf::sleep(sf::Time::Zero);
	}
	
	int Movie_video::getStreamID(void) const
	{
		return m_streamID;
	}
	
	const sf::Vector2i& Movie_video::getSize(void) const
	{
		return m_size;
	}
	
	sf::Time Movie_video::getWantedFrameTime(void) const
	{
		return m_wantedFrameTime;
	}
	
	sf::Image Movie_video::getImageCopy(void) const
	{
		return m_tex.copyToImage();
	}
	
	void Movie_video::decode(void)
	{
		while (m_runThread &&
			   m_running.waitAndLock(1, Condition::AutoUnlock) &&
			   m_backImageReady.waitAndLock(0))
		{
			sf::Time waitTime;
			bool isLate = getLateState(waitTime);
			
			if (loadNextImage(isLate))
			{
				m_backImageReady.unlock(1);
			}
			else
			{
				m_backImageReady.unlock(0);
			}
			
			if (m_isStarving)
			{
				m_parent.starvation();
			}
		}
	}
	
	bool Movie_video::getLateState(sf::Time& waitTime) const
	{
		bool flag = false;
		waitTime = sf::Time::Zero;
		// Get the 'real' time from the start of the video
		// and the progress of the video
		
		// Here is the real time elapsed since we started to play the video
		sf::Time realTime = m_parent.getPlayingOffset();
		
		// Here is the time we're at in the video
		// Note: m_wantedFrameTime is kept as float (seconds) for accuracy
		sf::Time movieTime = (sf::Int64)m_displayedFrameCount * m_wantedFrameTime;
		
		if (movieTime > realTime + m_wantedFrameTime)
		{
			flag = false;
			
			// Added a check to prevent from waiting if we've stopped
			// the movie playback (and thus waiting for abnormal periods of time)
			if (m_parent.getStatus() == Movie::Playing)
				waitTime = movieTime - realTime - m_wantedFrameTime;
		}
		else
		{
			// don't skip a frame if we just have one frame late,
			// it may be because of a occasional slowdown
			if (movieTime < realTime - m_wantedFrameTime && m_decodingTime > sf::Time::Zero)
			{
				flag = true;
				
				if (Movie::usesDebugMessages())
					std::cerr << "Movie_video::Run() - warning: skipping frame because we are late by " << sf::Time(realTime - movieTime).asMilliseconds() << "ms (movie playing offset is " << realTime.asMilliseconds() << "ms)" << std::endl;
			}
			else if (movieTime < realTime - m_wantedFrameTime && m_decodingTime > sf::Time::Zero)
			{
				if (Movie::usesDebugMessages())
					std::cerr << "Movie_video::Run() - warning: movie playback is late by " << sf::Time(realTime - movieTime).asMilliseconds() << "ms (movie playing offset is " << realTime.asMilliseconds() << "ms) but we're not skipping any frame since we're not 'too' late" << std::endl;
			}
		}
		
		return flag;
	}
	
	
	bool Movie_video::isStarving(void)
	{
		return m_isStarving;
	}
	
	
	void Movie_video::setPlayingOffset(sf::Time time)
	{
		stop();
		
		// TODO: does not work yet
		AVRational tb = m_parent.getAVFormatContext()->streams[m_streamID]->time_base;
		float ftb = (float)tb.num / tb.den;
		int64_t avTime = time.asMilliseconds() * ftb;
		int res = av_seek_frame(m_parent.getAVFormatContext(), m_streamID, avTime, AVSEEK_FLAG_BACKWARD);
		
		if (res < 0)
		{
			std::cerr << "Movie_video::SeekToTime() - av_seek_frame() failed" << std::endl;
		}
		else
		{
			play();
			m_displayedFrameCount = time.asSeconds() / m_wantedFrameTime.asSeconds();
		}
	}
	
	
	
	bool Movie_video::preLoad(void)
	{
		int counter = 0;
		bool res = false;
		while (false == (res = loadNextImage(false)) && counter < 10) // First frame always gives "frame not decoded"
			counter++;
		
		// Abort if we can't load frames
		if (counter == 10)
			return false;
		
		// Load first image
		loadNextImage(false);
		m_tex.update((sf::Uint8*)m_backRGBAFrame->data[0]);
		
		m_backImageReady = 1;
		return true;
	}
	
	
	bool Movie_video::loadNextImage(bool isLate)
	{
		bool flag = false;
		m_timer.restart();
		
		// If our video packet list is empty, load one more video frame
		if (!hasPendingDecodableData())
		{
			if (!readFrame())
			{
			    // Stop if there is no more data to read
				if (Movie::usesDebugMessages())
					std::cerr << "Movie_video::Update() - end of video stream reached." << std::endl;
				
				m_isStarving = true;
			}
			else
				flag = decodeFrontFrame(isLate);
		}
		else
		{
			flag = decodeFrontFrame(isLate);
		}
		
		m_decodingTime = m_timer.getElapsedTime();
		
		return flag;
	}
	
	bool Movie_video::readFrame(void)
	{
		while (!hasPendingDecodableData() &&
			   m_parent.readFrameAndQueue());
		
		return hasPendingDecodableData();
	}
	
	bool Movie_video::hasPendingDecodableData(void)
	{
		return (m_packetList.size() > 0);
	}
	
	bool Movie_video::decodeFrontFrame(bool isLate)
	{
		// whole function takes about 50% CPU with 2048x872 definition on Mac OS X
		// 50% (one full core) on Windows
		int didDecodeFrame = 0;
		bool flag = false;
		
		// Stop here if there is no frame to decode
		if (!hasPendingDecodableData())
		{
			if (Movie::usesDebugMessages())
				std::cerr << "Movie_video::DecodeFrontFrame() - no frame currently available for decoding" << std::endl;
			return flag;
		}
		
		// Get the front frame and decode it
		AVPacket *videoPacket = frontFrame();
		int res;
		res = avcodec_decode_video2(m_codecCtx, m_rawFrame, &didDecodeFrame,
									videoPacket); // 20% (40% of total function) on macosx; 18.3% (36% of total) on windows
		
		if (!isLate)
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
					m_imageSwapMutex.lock();
					sws_scale(m_swsCtx,
							  m_rawFrame->data, m_rawFrame->linesize,
							  0, m_codecCtx->height,
							  m_backRGBAFrame->data, m_backRGBAFrame->linesize);
					// 6.3% on windows (12% of total), 9.5% on Mac OS X
					
					m_imageSwapMutex.unlock();
					
					// Image loaded, reset condition state
					flag = true;
				}
				else
				{
					if (m_parent.Movie::usesDebugMessages())
						std::cerr << "Movie_video::DecodeFrontFrame() - frame not decoded" << std::endl;
				}
			}
		}
		
		popFrame();
		m_displayedFrameCount++;
		
		return flag;
	}
	
	void Movie_video::pushFrame(AVPacket *pkt)
	{
		sf::Lock l(m_packetListMutex);
		m_packetList.push(pkt);
	}
	
	void Movie_video::popFrame(void)
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
	
	AVPacket *Movie_video::frontFrame(void)
	{
		assert(!m_packetList.empty());
		
		sf::Lock l(m_packetListMutex);
		return m_packetList.front();
	}
	
} // namespace sfe

