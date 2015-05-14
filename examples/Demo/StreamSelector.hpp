
/*
 *  Streams.hpp
 *  sfeMovie project
 *
 *  Copyright (C) 2010-2015 Lucas Soltic
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

#ifndef SFEMOVIE_SAMPLE_STREAM_SELECTOR_HPP
#define SFEMOVIE_SAMPLE_STREAM_SELECTOR_HPP

#include <sfeMovie/Movie.hpp>
#include <map>

class StreamSelector
{
public:
    /** Initial setup is to select the first video and audio stream, and no subtitle stream
     */
    StreamSelector(sfe::Movie& movie);
    
    /** Select the next found stream of the given type from the movie given at construction
     *
     * Once all of the streams of a specific kind have been iterated,
     * next call to this method unselect the current stream of the given type
     */
    void selectNextStream(sfe::MediaType type);
    
private:
    sfe::Movie& m_movie;
    std::map<sfe::MediaType, int> m_selectedStreamIndexes;
    std::map<sfe::MediaType, const sfe::Streams*> m_streams;
};

#endif
