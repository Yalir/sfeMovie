
/*
 *  MediaInfo.cpp
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

#include "MediaInfo.hpp"
#include <string>
#include <iostream>

namespace
{
    std::string StatusToString(sfe::Status status)
    {
        switch (status)
        {
            case sfe::Stopped:  return "Stopped";
            case sfe::Paused:   return "Paused";
            case sfe::Playing:  return "Playing";
            default:            return "unknown status";
        }
    }
}

std::string mediaTypeToString(sfe::MediaType type)
{
    switch (type)
    {
        case sfe::Audio:    return "audio";
        case sfe::Subtitle: return "subtitle";
        case sfe::Video:    return "video";
        case sfe::Unknown:  return "unknown";
        default:            return "(null)";
    }
}

void displayMediaInfo(const sfe::Movie& movie)
{
    std::cout << "Status: " << StatusToString(movie.getStatus()) << std::endl;
    std::cout << "Position: " << movie.getPlayingOffset().asSeconds() << "s" << std::endl;
    std::cout << "Duration: " << movie.getDuration().asSeconds() << "s" << std::endl;
    std::cout << "Size: " << movie.getSize().x << "x" << movie.getSize().y << std::endl;
    std::cout << "Framerate: " << movie.getFramerate() << " FPS (average)" << std::endl;
    std::cout << "Volume: " << movie.getVolume() << std::endl;
    std::cout << "Sample rate: " << movie.getSampleRate() << " Hz" << std::endl;
    std::cout << "Channel count: " << movie.getChannelCount() << std::endl;
    
    const sfe::Streams& videoStreams = movie.getStreams(sfe::Video);
    const sfe::Streams& audioStreams = movie.getStreams(sfe::Audio);
    const sfe::Streams& subtitleStreams = movie.getStreams(sfe::Subtitle);
    
    std::cout << videoStreams.size() + audioStreams.size() + subtitleStreams.size() << " streams found in the media" << std::endl;
    
    for (const sfe::StreamDescriptor& descriptor : videoStreams)
        std::cout << " #" << descriptor.identifier << " : " << mediaTypeToString(descriptor.type) << std::endl;
    
    for (const sfe::StreamDescriptor& descriptor : audioStreams)
    {
        std::cout << " #" << descriptor.identifier << " : " << mediaTypeToString(descriptor.type);
        
        if (!descriptor.language.empty())
            std::cout << " (language: " << descriptor.language << ")";
        std::cout << std::endl;
    }
    
    for (const sfe::StreamDescriptor& descriptor : subtitleStreams)
    {
        std::cout << " #" << descriptor.identifier << " : " << mediaTypeToString(descriptor.type);
        
        if (!descriptor.language.empty())
            std::cout << " (language: " << descriptor.language << ")";
        std::cout << std::endl;
    }
}
