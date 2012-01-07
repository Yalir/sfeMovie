
/*
 *  Movie.cpp
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

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include "Movie.h"
#include "Condition.h"
#include "Movie_video.h"
#include "Movie_audio.h"
#include "utils.h"
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
	m_duration(0),
	m_overallTimer(),
	m_progressAtPause(0),
	m_video(new Movie_video(*this)),
	m_audio(new Movie_audio(*this)),
	m_watchThread(&Movie::Watch, this),
	m_shouldStopCond(new Condition())
	{
	}

	Movie::~Movie(void)
	{
		Stop();
		Close();
		delete m_video;
		delete m_audio;
		delete m_shouldStopCond;
	}

	bool Movie::OpenFromFile(const std::string& filename)
	{
		int err = 0;
		bool preloaded = false;

		// Make sure everything is cleaned before opening a new movie
		Stop();
		Close();
		
		// Load all the decoders
		av_register_all();

		// Open the movie file
		err = av_open_input_file(&m_avFormatCtx, filename.c_str(), NULL, 0, NULL);

		if (err != 0)
		{
			OutputError(err, "unable to open file " + filename);
			return false;
		}

		// Read the general movie informations
		err = av_find_stream_info(m_avFormatCtx);

		if (err < 0)
		{
			OutputError(err);
			Close();
			return false;
		}

		if (UsesDebugMessages())
			// Output the movie informations
			dump_format(m_avFormatCtx, 0, filename.c_str(), 0);

		// Perform the audio and video loading
		m_hasVideo = m_video->Initialize();
		m_hasAudio = m_audio->Initialize();
		
		if (m_hasVideo)
		{
			preloaded = m_video->PreLoad();
			
			if (!preloaded) // Loading first frames failed
			{
				if (sfe::Movie::UsesDebugMessages())
					std::cerr << "Movie::OpenFromFile() - Movie_video::PreLoad() failed.\n";
			}
		}
		
		return m_hasAudio || (m_hasVideo && preloaded);
	}

	void Movie::Play(void)
	{
		if (m_status != Playing)
		{
			m_overallTimer.Reset();
			IFAUDIO(m_audio->Play());
			IFVIDEO(m_video->Play());
			
			// Don't restart watch thread if we're resuming
			if (m_status != Paused)
			{
				*m_shouldStopCond = 0;
				m_shouldStopCond->Restore();
				m_watchThread.Launch();
			}
			
			m_status = Playing;
		}
	}

	void Movie::Pause(void)
	{
		if (m_status == Playing)
		{
			// Prevent audio from being late compared to video
			// (audio usually gets a bit later each time you pause and resume
			// the movie playback, thus fix the video timing according
			// to the audio's one)
			// NB: Calling Pause()/Play() is the only way to resynchronize
			// audio and video when audio gets late for now.
			if (HasAudioTrack())
			{
				m_progressAtPause = m_audio->GetPlayingOffset();
				//std::cout << "synch according to audio track=" << m_progressAtPause << std::endl;
			}
			else
			{
				//std::cout << "synch according to progrAtPse=" << m_progressAtPause << " + elapsdTme=" << m_overallTimer.GetElapsedTime() << std::endl;
				m_progressAtPause += m_overallTimer.GetElapsedTime();
			}
			
			m_status = Paused;
			IFAUDIO(m_audio->Pause());
			IFVIDEO(m_video->Pause());
		}
	}

	void Movie::Stop(void)
	{
		InternalStop(false);
	}
	
	void Movie::InternalStop(bool calledFromWatchThread)
	{
		// prevent Stop() from being executed while already stopping from another thread
		sf::Lock l(m_stopMutex);
		
		if (m_status != Stopped)
		{
			m_status = Stopped;
			IFAUDIO(m_audio->Stop());
			IFVIDEO(m_video->Stop());
			m_progressAtPause = 0;
			SetEofReached(false);
			m_shouldStopCond->Invalidate();
			if (!calledFromWatchThread)
				m_watchThread.Wait();
		}
	}

	bool Movie::HasVideoTrack(void) const
	{
		return m_hasVideo;
	}

	bool Movie::HasAudioTrack(void) const
	{
		return m_hasAudio;
	}

	void Movie::SetVolume(float volume)
	{
		IFAUDIO(m_audio->SetVolume(volume));
	}

	float Movie::GetVolume(void) const
	{
		float volume = 0;
		IFAUDIO(volume = m_audio->GetVolume());
		return volume;
	}

	sf::Uint32 Movie::GetDuration(void) const
	{
		return m_duration;
	}

	sf::Vector2i Movie::GetSize(void) const
	{
		return m_video->GetSize();
	}

	void Movie::ResizeToFrame(int x, int y, int width, int height, bool preserveRatio)
	{
		ResizeToFrame(sf::IntRect(x, y, x + width, y + height), preserveRatio);
	}

	void Movie::ResizeToFrame(sf::IntRect frame, bool preserveRatio)
	{
		sf::Vector2i movie_size = GetSize();
		sf::Vector2i wanted_size = sf::Vector2i(frame.Width, frame.Height);
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

			SetScale((float)target_size.x / movie_size.x, (float)target_size.y / movie_size.y);
			new_size = target_size;
		}
		else
		{
			SetScale((float)wanted_size.x / movie_size.x, (float)wanted_size.y / movie_size.y);
			new_size = wanted_size;
		}

		SetPosition(frame.Left + (wanted_size.x - new_size.x) / 2,
					frame.Top + (wanted_size.y - new_size.y) / 2);
	}

	float Movie::GetFramerate(void) const
	{
		return 1 / m_video->GetWantedFrameTime();
	}

	unsigned int Movie::GetSampleRate(void) const
	{
		unsigned rate = 0;
		IFAUDIO(rate = m_audio->GetSampleRate());
		return rate;
	}

	unsigned int Movie::GetChannelCount(void) const
	{
		unsigned count = 0;
		IFAUDIO(count = m_audio->GetChannelCount());
		return count;
	}

	Movie::Status Movie::GetStatus() const
	{
		return m_status;
	}

	/*void Movie::SetPlayingOffset(float position)
	 #error change floag to int
	{
		PrintWithTime("offset before : " + s(GetPlayingOffset()));

		IFAUDIO(m_audio->SetPlayingOffset(position));
		IFVIDEO(m_video->SetPlayingOffset(position));
		m_progressAtPause = position;
		m_overallTimer.Reset();

		PrintWithTime("offset after : " + s(GetPlayingOffset()));
	}*/

	sf::Uint32 Movie::GetPlayingOffset() const
	{
		sf::Uint32 offset = 0;

		if (m_status == Playing)
			offset = m_progressAtPause + m_overallTimer.GetElapsedTime();
		else
			offset = m_progressAtPause;

		return offset;
	}

	sf::Image Movie::GetImageCopy(void) const
	{
		return m_video->GetImageCopy();
	}

	void Movie::UseDebugMessages(bool flag)
	{
		g_usesDebugMessages = flag;

		if (g_usesDebugMessages)
			av_log_set_level(AV_LOG_VERBOSE);
		else
			av_log_set_level(AV_LOG_ERROR);
	}

	
	void Movie::Draw(sf::RenderTarget& target, sf::RenderStates states) const
	{
		states.Transform *= GetTransform();
		m_video->Draw(target, states);
	}

	void Movie::OutputError(int err, const std::string& fallbackMessage)
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

	void Movie::Close(void)
	{
		IFVIDEO(m_video->Close());
		IFAUDIO(m_audio->Close());

		if (m_avFormatCtx) av_close_input_file(m_avFormatCtx);
		m_hasAudio = false;
		m_hasVideo = false;
		m_eofReached = false;
		m_status = Stopped;
		m_duration = 0;
		m_progressAtPause = 0;
	}

	AVFormatContext *Movie::GetAVFormatContext(void)
	{
		return m_avFormatCtx;
	}

	bool Movie::GetEofReached()
	{
		return m_eofReached;
	}

	void Movie::SetEofReached(bool flag)
	{
		m_eofReached = flag;
	}

	void Movie::SetDuration(sf::Uint32 duration)
	{
		m_duration = duration;
	}
	 
	bool Movie::ReadFrameAndQueue(void)
	{
		// Avoid reading from different threads at the same time
		sf::Lock l(m_readerMutex);
		bool flag = true;
		AVPacket *pkt = NULL;
		
		// check we're not at eof
		if (GetEofReached())
			flag = false;
		else
		{	
			// read frame
			pkt = (AVPacket *)av_malloc(sizeof(*pkt));
			av_init_packet(pkt);
			
			int res = av_read_frame(GetAVFormatContext(), pkt);
			
			// check we didn't reach eof right now
			if (res < 0)
			{
				SetEofReached(true);
				flag = false;
				av_free_packet(pkt);
				av_free(pkt);
			}
			else
			{
				// When a frame has been read, save it
				if (!SaveFrame(pkt))
				{
					if (Movie::UsesDebugMessages())
						std::cerr << "Movie::ReadFrameAndQueue() - did read unknown packet type" << std::endl;
					av_free_packet(pkt);
					av_free(pkt);
				}
			}
		}
		
		return flag;
	}
	
	bool Movie::SaveFrame(AVPacket *frame)
	{
		bool saved = false;

		if (m_hasAudio && frame->stream_index == m_audio->GetStreamID())
		{
			// If it was an audio frame...
			m_audio->PushFrame(frame);
			saved = true;
		}
		else if (m_hasVideo && frame->stream_index == m_video->GetStreamID())
		{
			// If it was a video frame...
			m_video->PushFrame(frame);
			saved = true;
		}
		else
		{
			if (UsesDebugMessages())
				std::cerr << "Movie::SaveFrame() - unknown packet stream id ("
				<< frame->stream_index << ")\n";
		}

		return saved;
	}

	bool Movie::UsesDebugMessages(void)
	{
		return g_usesDebugMessages;
	}
	
	void Movie::Starvation(void)
	{
		bool audioStarvation = true;
		bool videoStarvation = true;
		
		IFAUDIO(audioStarvation = m_audio->IsStarving());
		IFVIDEO(videoStarvation = m_video->IsStarving());
		
		// No mode audio or video data to read
		if (audioStarvation && videoStarvation)
		{
			*m_shouldStopCond = 1;
		}
	}
	
	void Movie::Watch(void)
	{
		if (m_shouldStopCond->WaitAndLock(1, Condition::AutoUnlock))
		{
			InternalStop(true);
		}
	}

} // namespace sfe

