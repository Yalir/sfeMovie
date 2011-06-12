
/*
 *  Movie.h
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


#ifndef MOVIE_H
#define MOVIE_H

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Mutex.hpp>
#include <SFML/System/Thread.hpp>
#include <string>


namespace sfe {
	class Movie_audio;
	class Movie_video;
	class Condition;
	
	class Movie : public sf::Drawable {
		friend class Movie_audio;
		friend class Movie_video;
	public:
		enum Status
		{
			Stopped, // Movie is not playing
			Paused,  // Movie is paused
			Playing  // Movie is playing
		};
		
		/** Default constructor
		 */
		Movie(void);
		
		/** Default destructor
		 */
		~Movie(void);
		
		/** Attemps to read a movie file.
		 * @filename: the path to the movie file
		 * @return: true on success, false otherwise
		 */
		bool OpenFromFile(const std::string& filename);
		
		/** Starts the reading of the movie file
		 */
		void Play(void);
		
		/** Pauses the reading of the movie file. If the movie playback is already paused,
		 * this does nothing, otherwise the playback is paused.
		 */
		void Pause(void);
		
		/** Stops the reading of the movie file. The playing offset is then reset to 0.
		 */
		void Stop(void);
		
		/** Returns whether the opened movie contains a video track (images)
		 * @return: true if the opened movie contains a video track, false otherwise
		 */
		bool HasVideoTrack(void) const;
		
		/** Returns whether the opened movie contains an audio track
		 * @return: true if the opened movie contains an audio track, false otherwise
		 */
		bool HasAudioTrack(void) const;
		
		/** Sets the sound volume (default is 100)
		 * @volume: the volume in range [0, 100]
		 */
		void SetVolume(float volume);
		
		/** Returns the current sound volume
		 * @return: the sound's volume, in range [0, 100]
		 */
		float GetVolume(void) const;
		
		/** Returns the duration of the movie
		 * @return: the duration in milliseconds
		 */
		sf::Uint32 GetDuration(void) const;
		
		/** Returns the size (width/height) of the movie
		 * @return: the size of the movie
		 */
		sf::Vector2i GetSize(void) const;
		
		/** See ResizeToFrame(sf::IntRect, bool)
		 */
		void ResizeToFrame(int x, int y, int width, int height, bool preserveRatio = true);
		
		/** Scales the movie to fit the requested frame.
		 * If the ratio is preserved, the movie may be centered
		 * in the given frame. Thus you're not guaranted that
		 * the movie position won't be changed.
		 * @frame: the target frame in which you want to display the movie
		 * @preserveRatio: true to keep the original movie ratio, false otherwise
		 */
		void ResizeToFrame(sf::IntRect frame, bool preserveRatio = true);
		
		/** Returns the amount of video frames per second
		 * @return: the video frame rate
		 */
		float GetFramerate(void) const;
		
		/** Returns the amount of audio samples per second
		 * @return: the audio sample rate
		 */
		unsigned int GetSampleRate(void) const;
		
		/** Returns the count of audio channels
		 * @return: the channels' count
		 */
		unsigned int GetChannelsCount(void) const;
		
		/** Returns the current status of this movie
		 * @return: See enum Status
		 */
		Status GetStatus(void) const;
		
		/** Sets the current playing position in the movie
		 * @return: the playing position, in milliseconds
		 * NOTE: Not yet implemented! 
		 */
		//void SetPlayingOffset(sf::Uint32 position);
		
		/** Returns the current playing position in the movie
		 * @return: the playing position, in milliseconds
		 */
		sf::Uint32 GetPlayingOffset() const;
		
		/** Returns a copy of the image currently being displayed.
		 * If the movie has no video track, this returns an empty image.
		 * Note that, as the returned image is a copy, modifying it
		 * has no effect of the movie being displayed.
		 * @return: the current image of the movie
		 */
		sf::Image GetImageCopy(void) const;
		
		//void SetLoop(bool Loop);
		//bool GetLoop() const;
		
		/** Enables or disables the debug messages outputting
		 * When enabled, the following debug messages can be dispayed:
		 * - the attributes of the opened movie 
		 * - a notification message when a frame was not decoded
		 * - a notification message when a frame has been skipped
		 * (because the movie playback was late)
		 *
		 * Disabling the debug messages does not prevent sfe::Movie from
		 * displaying the error messages.
		 * The debug messages are always sent to the cerr output stream.
		 *
		 * @flag: true to enable the debug messages outputting, false otherwise
		 */
		static void UseDebugMessages(bool flag = true);
		
	private:
		
#ifndef LIBAVCODEC_VERSION
		typedef void *AVFormatContextRef;
		typedef void *AVPacketRef;
#else
		typedef AVFormatContext *AVFormatContextRef;
		typedef AVPacket *AVPacketRef;
#endif
		void InternalStop(bool calledFromWatchThread);
		void Render(sf::RenderTarget& Target, sf::Renderer& renderer) const;
		
		static void OutputError(int err, const std::string& fallbackMessage = "");
		void Close(void);
		
		AVFormatContextRef GetAVFormatContext(void);
		bool GetEofReached();
		void SetEofReached(bool flag);
		void SetDuration(sf::Uint32 duration);
		bool ReadFrameAndQueue(void);
		bool SaveFrame(AVPacketRef frame);
		static bool UsesDebugMessages(void);
		void Starvation(void);
		void Watch(void);
		
		AVFormatContextRef m_avFormatCtx;
		bool m_hasVideo;
		bool m_hasAudio;
		bool m_eofReached;
		sf::Mutex m_stopMutex;
		sf::Mutex m_readerMutex;
		sf::Thread m_watchThread;
		Condition *m_shouldStopCond;
		
		Status m_status;
		sf::Uint32 m_duration;
		sf::Clock m_overallTimer;
		sf::Uint32 m_progressAtPause;
		
		Movie_video *m_video;
		Movie_audio *m_audio;
		
		static bool g_usesDebugMessages;
	};
	
} // namespace sfe

#endif
