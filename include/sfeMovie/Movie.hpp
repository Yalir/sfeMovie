
/*
 *  Movie.hpp
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


#ifndef MOVIE_HPP
#define MOVIE_HPP

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Mutex.hpp>
#include <SFML/System/Thread.hpp>
#include <SFML/Config.hpp>
#include <string>


////////////////////////////////////////////////////////////
// Define portable import / export macros
////////////////////////////////////////////////////////////
#if defined(SFML_SYSTEM_WINDOWS) && defined(_MSC_VER)
    #ifdef SFE_EXPORTS
        // From DLL side, we must export
        #define SFE_API __declspec(dllexport)
    #else
        // From client application side, we must import
        #define SFE_API __declspec(dllimport)
    #endif

    // For Visual C++ compilers, we also need to turn off this annoying C4251 warning.
    // You can read lots ot different things about it, but the point is the code will
    // just work fine, and so the simplest way to get rid of this warning is to disable it
    #ifdef _MSC_VER
        #pragma warning(disable : 4251)
    #endif
#else
	#define SFE_API
#endif

namespace sfe {
	class Movie_audio;
	class Movie_video;
	class Condition;
	
	class SFE_API Movie : public sf::Drawable, public sf::Transformable {
		friend class Movie_audio;
		friend class Movie_video;
	public:
		/** @brief Constants giving the movie playback status
		 */
		enum Status
		{
			Stopped, //!< Movie is stopped (ie. not playing and at the beginning)
			Paused,  //!< Movie is paused
			Playing  //!< Movie is playing
		};
		
		/** @brief Default constructor
		 */
		Movie(void);
		
		/** @brief Default destructor
		 */
		~Movie(void);
		
		/** @brief Attemps to open a media file (movie or audio)
		 *
		 * Opening can fails either because of a wrong filename,
		 * or because you tried to open a movie file that has unsupported
		 * video and audio format.
		 *
		 * @param filename the path to the movie file
		 * @return true on success, false otherwise
		 */
		bool openFromFile(const std::string& filename);
		
		/** @brief Starts the movie playback
		 */
		void play(void);
		
		/** @brief Pauses the movie playback
		 *
		 * If the movie playback is already paused,
		 * this does nothing, otherwise the playback is paused.
		 */
		void pause(void);
		
		/** @brief Stops the movie playback. The playing offset is reset to the beginning.
		 */
		void stop(void);
		
		/** @brief Returns whether the opened movie contains a video track (images)
		 *
		 * @return true if the opened movie contains a video track, false otherwise
		 */
		bool hasVideoTrack(void) const;
		
		/** @brief Returns whether the opened movie contains an audio track
		 *
		 * @return true if the opened movie contains an audio track, false otherwise
		 */
		bool hasAudioTrack(void) const;
		
		/** @brief Sets the sound's volume (default is 100)
		 *
		 * @param volume the volume in range [0, 100]
		 */
		void setVolume(float volume);
		
		/** @brief Returns the current sound's volume
		 *
		 * @return the sound's volume, in range [0, 100]
		 */
		float getVolume(void) const;
		
		/** @brief Returns the duration of the movie
		 *
		 * @return the duration in milliseconds
		 */
		sf::Time getDuration(void) const;
		
		/** @brief Returns the size (width, height) of the movie
		 *
		 * @return the size of the movie
		 */
		sf::Vector2i getSize(void) const;
		
		/** @brief See resizeToFrame(sf::IntRect, bool)
		 * @see resizeToFrame(sf::IntRect, bool)
		 */
		void resizeToFrame(int x, int y, int width, int height, bool preserveRatio = true);
		
		/** @brief Scales the movie to fit the requested frame.
		 *
		 * If the ratio is preserved, the movie may be centered
		 * in the given frame. Thus the movie position may be different from
		 * the one you specified.
		 * @param frame the target frame in which you want to display the movie
		 * @param preserveRatio true to keep the original movie ratio, false otherwise
		 */
		void resizeToFrame(sf::IntRect frame, bool preserveRatio = true);
		
		/** @brief Returns the amount of video frames per second
		 *
		 * @return the video frame rate
		 */
		float getFramerate(void) const;
		
		/** @brief Returns the amount of audio samples per second
		 *
		 * @return the audio sample rate
		 */
		unsigned int getSampleRate(void) const;
		
		/** @brief Returns the count of audio channels
		 *
		 * @return the channels' count
		 */
		unsigned int getChannelCount(void) const;
		
		/** @brief Returns the current status of the movie
		 *
		 * @return See enum Status
		 */
		Status getStatus(void) const;
		
		/* @brief Sets the current playing position in the movie
		 *
		 * @return the playing position, in milliseconds
		 * NOTE: Not yet implemented! 
		 */
		//void SetPlayingOffset(sf::Uint32 position);
		
		/** @brief Returns the current playing position in the movie
		 *
		 * @return the playing position
		 */
		sf::Time getPlayingOffset(void) const;
		
		/** @brief Returns a const reference to the movie texture currently being displayed.
		 *
		 * The returned image is a texture in VRAM.
		 * Note: although the returned texture reference remains the same,
		 * getCurrentFrame() must be called for each new frame until you also use
		 * draw() ; otherwise the texture won't be updated.
		 *
		 * If the movie has no video track, this returns an empty image.
		 * @return the current image of the movie
		 */
		const sf::Texture& getCurrentFrame(void) const;
		
		//void SetLoop(bool Loop);
		//bool GetLoop() const;
		
		/** @brief Choose whether to print debug messages
		 *
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
		 * @param flag true to enable the debug messages outputting, false otherwise
		 */
		static void useDebugMessages(bool flag = true);
		
	private:
		
#ifndef LIBAVCODEC_VERSION
		typedef void *AVFormatContextRef;
		typedef void *AVPacketRef;
#else
		typedef AVFormatContext *AVFormatContextRef;
		typedef AVPacket *AVPacketRef;
#endif
		void internalStop(bool calledFromWatchThread);
		void draw(sf::RenderTarget& Target, sf::RenderStates states) const;
		
		static void outputError(int err, const std::string& fallbackMessage = "");
		void close(void);
		
		AVFormatContextRef getAVFormatContext(void);
		bool getEofReached();
		void setEofReached(bool flag);
		void setDuration(sf::Time duration);
		bool readFrameAndQueue(void);
		bool saveFrame(AVPacketRef frame);
		static bool usesDebugMessages(void);
		void starvation(void);
		void watch(void);
		
		AVFormatContextRef m_avFormatCtx;
		bool m_hasVideo;
		bool m_hasAudio;
		bool m_eofReached;
		sf::Mutex m_stopMutex;
		sf::Mutex m_readerMutex;
		sf::Thread m_watchThread;
		Condition *m_shouldStopCond;
		
		Status m_status;
		sf::Time m_duration;
		sf::Clock m_overallTimer;
		sf::Time m_progressAtPause;
		
		Movie_video *m_video;
		Movie_audio *m_audio;
	};
	
} // namespace sfe

#endif
