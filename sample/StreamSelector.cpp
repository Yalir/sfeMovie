
/*
 *  Streams.cpp
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

#include "StreamSelector.hpp"

StreamSelector::StreamSelector(sfe::Movie& movie)
: m_movie(movie)
, m_selectedStreamIndexes()
, m_streams()
{
    m_selectedStreamIndexes[sfe::Video] = 0;
    m_selectedStreamIndexes[sfe::Audio] = 0;
    m_selectedStreamIndexes[sfe::Subtitle] = -1;
    
    m_streams[sfe::Video] = &movie.getStreams(sfe::Video);
    m_streams[sfe::Audio] = &movie.getStreams(sfe::Audio);
    m_streams[sfe::Subtitle] = &movie.getStreams(sfe::Subtitle);
    
    if (m_selectedStreamIndexes[sfe::Video] >= 0 &&
        m_selectedStreamIndexes[sfe::Video] < m_streams[sfe::Video]->size())
        movie.selectStream(m_streams[sfe::Video]->at(m_selectedStreamIndexes[sfe::Video]));
    
    if (m_selectedStreamIndexes[sfe::Audio] >= 0 &&
        m_selectedStreamIndexes[sfe::Audio] < m_streams[sfe::Audio]->size())
        movie.selectStream(m_streams[sfe::Audio]->at(m_selectedStreamIndexes[sfe::Audio]));
    
    if (m_selectedStreamIndexes[sfe::Subtitle] >= 0 &&
        m_selectedStreamIndexes[sfe::Subtitle] < m_streams[sfe::Subtitle]->size())
        movie.selectStream(m_streams[sfe::Subtitle]->at(m_selectedStreamIndexes[sfe::Subtitle]));
}

void StreamSelector::selectNextStream(sfe::MediaType type)
{
    m_selectedStreamIndexes[type] = (m_selectedStreamIndexes[type] + 1) % (m_streams[type]->size() + 1);
    
    if (m_selectedStreamIndexes[type] == m_streams[type]->size())
        m_movie.selectStream(sfe::StreamDescriptor::NoSelection(type));
    else
        m_movie.selectStream(m_streams[type]->at(m_selectedStreamIndexes[type]));
}

