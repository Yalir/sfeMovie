
/*
 *  Movie.cpp
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

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include <sfeMovie/Movie.hpp>
#include "Condition.hpp"
#include "Barrier.hpp"
#include "Movie_video.hpp"
#include "Movie_audio.hpp"
#include "utils.hpp"
#include <SFML/Graphics.hpp>
#include <iostream>

#define IFAUDIO(sequence) { if (m_hasAudio) { sequence; } }
#define IFVIDEO(sequence) { if (m_hasVideo) { sequence; } }

namespace sfe {

	static bool g_usesDebugMessages = false;

	Movie::Movie(void) :
	m_avFormatCtx(NULL),
	m_hasVideo(false),
	m_hasAudio(false),
	m_eofReached(false),
	m_stopMutex(),
	m_status(Stopped),
	m_duration(sf::Time::Zero),
	m_overallTimer(),
	m_progressAtPause(sf::Time::Zero),
	m_video(new Movie_video(*this)),
	m_audio(new Movie_audio(*this)),
	m_watchThread(&Movie::watch, this),
	m_shouldStopCond(new Condition()),
	m_barrier(NULL),
	m_barrierMutex()
	{
	}

	Movie::~Movie(void)
	{
		stop();
		close();
		delete m_video;
		delete m_audio;
		delete m_shouldStopCond;
	}

	bool Movie::openFromFile(const std::string& filename)
	{
		int err = 0;
		bool preloaded = false;

		// Make sure everything is cleaned before opening a new movie
		stop();
		close();
		
		// Load all the decoders
		av_register_all();

		// Open the movie file
		err = avformat_open_input(&m_avFormatCtx, filename.c_str(), NULL, NULL);

		if (err != 0)
		{
			outputError(err, "unable to open file " + filename);
			return false;
		}

		// Read the general movie informations
		err = avformat_find_stream_info(m_avFormatCtx, NULL);

		if (err < 0)
		{
			outputError(err);
			close();
			return false;
		}

		if (usesDebugMessages())
			// Output the movie informations
			av_dump_format(m_avFormatCtx, 0, filename.c_str(), 0);

		// Perform the audio and video loading
		m_hasVideo = m_video->initialize();
		m_hasAudio = m_audio->initialize();
		
		if (m_hasVideo)
		{
			preloaded = m_video->preLoad();
			
			if (!preloaded) // Loading first frames failed
			{
				if (sfe::Movie::usesDebugMessages())
					std::cerr << "Movie::OpenFromFile() - Movie_video::PreLoad() failed.\n";
			}
		}
		
		if (m_hasAudio && m_hasAudio)
		{
			m_barrier = new Barrier(2);
		}
		
		return m_hasAudio || (m_hasVideo && preloaded);
	}

	void Movie::play(void)
	{
		if (m_status != Playing)
		{
			IFAUDIO(m_audio->play());
			IFVIDEO(m_video->play());
			
			readyToPlay();
			m_overallTimer.restart();
			
			if (usesDebugMessages())
				printWithTime("did start movie timer");
			
			// Don't restart watch thread if we're resuming
			if (m_status != Paused)
			{
				*m_shouldStopCond = 0;
				m_shouldStopCond->restore();
				m_watchThread.launch();
			}
			
			m_status = Playing;
		}
	}

	void Movie::pause(void)
	{
		if (m_status == Playing)
		{
			// Prevent audio from being late compared to video
			// (audio usually gets a bit later each time you pause and resume
			// the movie playback, thus fix the video timing according
			// to the audio's one)
			// NB: Calling Pause()/Play() is the only way to resynchronize
			// audio and video when audio gets late for now.
			if (hasAudioTrack())
			{
				m_progressAtPause = m_audio->getPlayingOffset();
				//std::cout << "synch according to audio track=" << m_progressAtPause << std::endl;
			}
			else
			{
				//std::cout << "synch according to progrAtPse=" << m_progressAtPause << " + elapsdTme=" << m_overallTimer.GetElapsedTime() << std::endl;
				m_progressAtPause += m_overallTimer.getElapsedTime();
			}
			
			m_status = Paused;
			IFAUDIO(m_audio->pause());
			IFVIDEO(m_video->pause());
		}
	}

	void Movie::stop(void)
	{
		internalStop(false);
	}
	
	void Movie::internalStop(bool calledFromWatchThread)
	{
		// prevent Stop() from being executed while already stopping from another thread
		sf::Lock l(m_stopMutex);
		
		if (m_status != Stopped)
		{
			m_status = Stopped;
			IFAUDIO(m_audio->stop());
			IFVIDEO(m_video->stop());
			
			m_progressAtPause = sf::Time::Zero;
			setEofReached(false);
			m_shouldStopCond->invalidate();
			
			if (!calledFromWatchThread)
				m_watchThread.wait();
		}
	}

	bool Movie::hasVideoTrack(void) const
	{
		return m_hasVideo;
	}

	bool Movie::hasAudioTrack(void) const
	{
		return m_hasAudio;
	}

	void Movie::setVolume(float volume)
	{
		IFAUDIO(m_audio->setVolume(volume));
	}

	float Movie::getVolume(void) const
	{
		float volume = 0;
		IFAUDIO(volume = m_audio->getVolume());
		return volume;
	}

	sf::Time Movie::getDuration(void) const
	{
		return m_duration;
	}

	sf::Vector2i Movie::getSize(void) const
	{
		return m_video->getSize();
	}

	void Movie::resizeToFrame(int x, int y, int width, int height, bool preserveRatio)
	{
		resizeToFrame(sf::IntRect(x, y, x + width, y + height), preserveRatio);
	}

	void Movie::resizeToFrame(sf::IntRect frame, bool preserveRatio)
	{
		sf::Vector2i movie_size = getSize();
		sf::Vector2i wanted_size = sf::Vector2i(frame.width, frame.height);
		sf::Vector2i new_size;

		if (preserveRatio)
		{
			sf::Vector2i target_size = movie_size;

			float source_ratio = (float)movie_size.x / movie_size.y;
			float target_ratio = (float)wanted_size.x / wanted_size.y;

			if (source_ratio > target_ratio)
			{
				target_size.x = movie_size.x * ((float)wanted_size.x / movie_size.x);
				target_size.y = movie_size.y * ((float)wanted_size.x / movie_size.x);
			}
			else
			{
				target_size.x = movie_size.x * ((float)wanted_size.y / movie_size.y);
				target_size.y = movie_size.y * ((float)wanted_size.y / movie_size.y);
			}

			setScale((float)target_size.x / movie_size.x, (float)target_size.y / movie_size.y);
			new_size = target_size;
		}
		else
		{
			setScale((float)wanted_size.x / movie_size.x, (float)wanted_size.y / movie_size.y);
			new_size = wanted_size;
		}

		setPosition(frame.left + (wanted_size.x - new_size.x) / 2,
					frame.top + (wanted_size.y - new_size.y) / 2);
	}

	float Movie::getFramerate(void) const
	{
		return 1. / m_video->getWantedFrameTime().asSeconds();
	}

	unsigned int Movie::getSampleRate(void) const
	{
		unsigned rate = 0;
		IFAUDIO(rate = m_audio->getSampleRate());
		return rate;
	}

	unsigned int Movie::getChannelCount(void) const
	{
		unsigned count = 0;
		IFAUDIO(count = m_audio->getChannelCount());
		return count;
	}

	Movie::Status Movie::getStatus() const
	{
		return m_status;
	}

	/*void Movie::SetPlayingOffset(float position)
	 #error change floag to int
	{
		PrintWithTime("offset before : " + s(GetPlayingOffset()));

		IFAUDIO(m_audio->setPlayingOffset(position));
		IFVIDEO(m_video->setPlayingOffset(position));
		m_progressAtPause = position;
		m_overallTimer.Reset();

		PrintWithTime("offset after : " + s(GetPlayingOffset()));
	}*/

	sf::Time Movie::getPlayingOffset() const
	{
		sf::Time offset = sf::Time::Zero;

		if (m_status == Playing)
			offset = m_progressAtPause + m_overallTimer.getElapsedTime();
		else
			offset = m_progressAtPause;

		return offset;
	}

	const sf::Texture& Movie::getCurrentFrame(void) const
	{
		static sf::Texture emptyTexture;
		
		if (m_hasVideo)
			return m_video->getCurrentFrame();
		else
			return emptyTexture;
	}

	void Movie::useDebugMessages(bool flag)
	{
		g_usesDebugMessages = flag;

		if (g_usesDebugMessages)
			av_log_set_level(AV_LOG_VERBOSE);
		else
			av_log_set_level(AV_LOG_ERROR);
	}

	
	void Movie::draw(sf::RenderTarget& target, sf::RenderStates states) const
	{
		states.transform *= getTransform();
		m_video->draw(target, states);
	}

	void Movie::outputError(int err, const std::string& fallbackMessage)
	{
		char buffer[4096] = {0};

		if (/*err != AVERROR_NOENT &&*/ 0 == av_strerror(err, buffer, sizeof(buffer)))
			std::cerr << "FFmpeg error: " << buffer << std::endl;
		else
		{
		    if (fallbackMessage.length())
                std::cerr << "FFmpeg error: " << fallbackMessage << std::endl;
            else
                std::cerr << "FFmpeg error: unable to retrieve the error message (and no fallback message set)" << std::endl;
		}
	}

	void Movie::close(void)
	{
		IFVIDEO(m_video->close());
		IFAUDIO(m_audio->close());

		if (m_avFormatCtx)
			avformat_close_input(&m_avFormatCtx);
		m_hasAudio = false;
		m_hasVideo = false;
		m_eofReached = false;
		m_status = Stopped;
		m_duration = sf::Time::Zero;
		m_progressAtPause = sf::Time::Zero;
		delete m_barrier;
	}

	AVFormatContext *Movie::getAVFormatContext(void)
	{
		return m_avFormatCtx;
	}

	bool Movie::getEofReached()
	{
		return m_eofReached;
	}

	void Movie::setEofReached(bool flag)
	{
		m_eofReached = flag;
	}

	void Movie::setDuration(sf::Time duration)
	{
		m_duration = duration;
	}
	 
	bool Movie::readFrameAndQueue(void)
	{
		// Avoid reading from different threads at the same time
		sf::Lock l(m_readerMutex);
		bool flag = true;
		AVPacket *pkt = NULL;
		
		// check we're not at eof
		if (getEofReached())
			flag = false;
		else
		{	
			// read frame
			pkt = (AVPacket *)av_malloc(sizeof(*pkt));
			av_init_packet(pkt);
			
			int res = av_read_frame(getAVFormatContext(), pkt);
			
			// check we didn't reach eof right now
			if (res < 0)
			{
				setEofReached(true);
				flag = false;
				av_free_packet(pkt);
				av_free(pkt);
			}
			else
			{
				// When a frame has been read, save it
				if (!saveFrame(pkt))
				{
					if (Movie::usesDebugMessages())
						std::cerr << "Movie::ReadFrameAndQueue() - did read unknown packet type" << std::endl;
					av_free_packet(pkt);
					av_free(pkt);
				}
			}
		}
		
		return flag;
	}
	
	bool Movie::saveFrame(AVPacket *frame)
	{
		bool saved = false;

		if (m_hasAudio && frame->stream_index == m_audio->getStreamID())
		{
			// If it was an audio frame...
			m_audio->pushFrame(frame);
			saved = true;
		}
		else if (m_hasVideo && frame->stream_index == m_video->getStreamID())
		{
			// If it was a video frame...
			m_video->pushFrame(frame);
			saved = true;
		}
		else
		{
			if (usesDebugMessages())
				std::cerr << "Movie::SaveFrame() - unknown packet stream id ("
				<< frame->stream_index << ")\n";
		}

		return saved;
	}

	bool Movie::usesDebugMessages(void)
	{
		return g_usesDebugMessages;
	}
	
	void Movie::starvation(void)
	{
		bool audioStarvation = true;
		bool videoStarvation = true;
		
		IFAUDIO(audioStarvation = m_audio->isStarving());
		IFVIDEO(videoStarvation = m_video->isStarving());
		
		// No mode audio or video data to read
		if (audioStarvation && videoStarvation)
		{
			*m_shouldStopCond = 1;
		}
	}
	
	void Movie::readyToPlay(void)
	{
		if (m_barrier)
		{
			m_barrier->wait();
			
			m_barrierMutex.lock();
			delete m_barrier;
			m_barrier = NULL;
			m_barrierMutex.unlock();
		}
	}
	
	void Movie::watch(void)
	{
		if (m_shouldStopCond->waitAndLock(1, Condition::AutoUnlock))
		{
			internalStop(true);
		}
	}

} // namespace sfe

