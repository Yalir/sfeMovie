
/*
 *  AudioStream.hpp
 *  sfeMovie project
 *
 *  Copyright (C) 2010-2014 Lucas Soltic
 *  lucas.soltic@orange.fr
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


#ifndef SFEMOVIE_AUDIOSTREAM_HPP
#define SFEMOVIE_AUDIOSTREAM_HPP

#include <SFML/Audio.hpp>
#include "Macros.hpp"
#include "Stream.hpp"

namespace sfe {
	class AudioStream : public Stream, private sf::SoundStream {
	public:
		/** Create a video stream from the given FFmpeg stream
		 *
		 * At the end of the constructor, the stream is guaranteed
		 * to have all of its fields set and the decoder loaded
		 */
		AudioStream(AVStreamRef stream, DataSource& dataSource);
		
		/** Default destructor
		 */
		virtual ~AudioStream(void);
		
		/* A/V control */
		
		/** Start playing this stream
		 */
		virtual void play(void);
		
		/** Pause stream playback
		 */
		virtual void pause(void);
		
		/** Stop stream playback and go back to beginning
		 */
		virtual void stop(void);
		
		/** Get the stream kind (either audio, video or subtitle stream)
		 *
		 * @return the kind of stream represented by this stream
		 */
		virtual Kind getStreamKind(void) const;
		
	private:
		virtual bool onGetData(sf::SoundStream::Chunk& data);
		virtual void onSeek(sf::Time timeOffset);
	};
}

#endif
