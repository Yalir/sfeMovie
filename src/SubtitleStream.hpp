
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


namespace sfe
{
	namespace{
		const int kRgbaSize = 4;
	}

	class SubtitleStream : public Stream{
	public:
		struct Delegate {
			virtual void didUpdateSubtitle(const SubtitleStream& sender, const std::vector<sf::Sprite>& subimages) = 0;
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
	private:
		/** The struct we use to store our subtitles
		*/
		struct SubImage
		{
			std::vector<sf::Sprite> out;
			int64_t pts;	
			AVSubtitle sub;
		};

		/**Convert an AVSubtitle to an RGBA image and set the position of the subtitle in the sprite
		*
		* @return A vector of sprites which will be rendered as the subtitles
		*/
		std::vector<sf::Sprite>  SubtitleToSprites(AVSubtitle* sub);

		/** Decode the packages that were send to the stream by the demuxer
		*
		* @return if the stream is finished or not
		*/
		bool onGetData();

		Delegate& m_delegate;
		std::list<sf::Texture> m_textures;
		sf::Text m_subtext;
		std::vector<SubImage> m_inactive;
		std::vector<SubImage> m_active;
	};

};

#endif