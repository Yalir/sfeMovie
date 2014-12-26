
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
#include <list>
#include <utility>


namespace sfe
{
    
    class SubtitleStream : public Stream
    {
    public:
        struct Delegate
        {
            /** Triggered by the subtitle stream when a new subtitle is available and should be displayed
             */
            virtual void didUpdateSubtitle(const SubtitleStream& sender,
                                           const std::list<sf::Sprite>& subimages,
                                           const std::list<sf::Vector2i>& positions) = 0;
            
            /** Triggered by the subtitle stream when buffers are flushed and the subtitles previously
             * sent for display are no more available
             */
            virtual void didWipeOutSubtitles(const SubtitleStream& sender) = 0;
        };
        
        /** Create a subtitle stream from the given FFmpeg stream
         *
         * At the end of the constructor, the stream is guaranteed
         * to have all of its fields set and the decoder loaded
         */
        SubtitleStream(AVFormatContext*& formatCtx, AVStream*& stream, DataSource& dataSource, std::shared_ptr<Timer> timer, Delegate& delegate);
        
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
        
        /** @see Stream::isPassive()
         */
        bool isPassive() const override;
    private:
        /** The struct we use to store our subtitles
         */
        struct SubtitleData
        {
            std::list<sf::Texture> textures;
            std::list<sf::Sprite> sprites;
            std::list<sf::Vector2i> positions;
            //when will it appear (absolute)
            sf::Time start;
            //when will it disappear (absolute)
            sf::Time end;
            
            /** Create our subtitle from an AVSubtitle
             *
             * @param succeeded Whether this structure contains valid decoded subtitles
             * after construction time
             */
            SubtitleData(AVSubtitle* sub, bool& succeeded);
        };
        
        /** Decode the packages that were send to the stream by the demuxer
         *
         * @return if the stream is finished or not
         */
        bool onGetData();
        virtual void flushBuffers();
        
        Delegate& m_delegate;
        
        std::list< std::shared_ptr<SubtitleData> > m_pendingSubtitles;
        std::list< std::shared_ptr<SubtitleData> > m_visibleSubtitles;
    };
    
};

#endif