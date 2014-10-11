
/*
*  SubtitleStream.hpp
*  sfeMovie project
*
*  Copyright (C) 2010-2014 Stephan Vedder
*  stephan.vedder@gmail.com
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


#ifndef SFEMOVIE_SUBTITLESTREAM_HPP
#define SFEMOVIE_SUBTITLESTREAM_HPP

#include "Macros.hpp"
#include "Stream.hpp"
#include <SFML/Graphics.hpp>
#include <stdint.h>
#include <list>
#include <vector>


namespace sfe
{

	class SubtitleStream : public Stream{
	public:
		struct Delegate {
			virtual void didUpdateSubtitle(const SubtitleStream& sender,
                                           const std::vector<sf::Sprite>& subimages,
                                           const std::vector<sf::Vector2u>& subSizes) = 0;
		};
		/** Create a subtitle stream from the given FFmpeg stream
		*
		* At the end of the constructor, the stream is guaranteed
		* to have all of its fields set and the decoder loaded
		*/
		SubtitleStream(AVFormatContext* formatCtx, AVStream* stream, DataSource& dataSource, Timer& timer, Delegate& delegate);

		/** Default destructor
		*/
		virtual ~SubtitleStream();

		/** Get the stream kind (either audio, video or subtitle stream)
		*
		* @return the kind of stream represented by this stream
		*/
		virtual MediaType getStreamKind() const;

		/** Update the stream's status
		*/
		virtual void update();

		// Timer::Observer interface
		void willPlay(const Timer &timer);
		void didPlay(const Timer& timer, sfe::Status pcreviousStatus);
		void didPause(const Timer& timer, sfe::Status previousStatus);
		void didStop(const Timer& timer, sfe::Status previousStatus);

	private:
		/** The struct we use to store our subtitles
		*/
		struct SubtitleData
		{
			std::vector<sf::Sprite> sprites;
			std::list<sf::Texture> textures;
			//when will it appear (absolut)
			sf::Time start;
			//when will it disappear (absolut)
			sf::Time end;
			//Create our subtitle from an AVSubtitle
			SubtitleData(AVSubtitle* sub);
		};



		/** Decode the packages that were send to the stream by the demuxer
		*
		* @return if the stream is finished or not
		*/
		bool onGetData();

		Delegate& m_delegate;
		
		
		std::list<SubtitleData*> m_inactive;
		std::list<SubtitleData*> m_active;
	};

};

#endif