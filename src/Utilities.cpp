
/*
 *  Utilities.cpp
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

#include "Utilities.hpp"
#include "Demuxer.hpp"
#include <set>
#include <utility>
#include <iostream>

namespace sfe {
	void dumpAvailableDemuxers(void)
	{
		const std::list<Demuxer::DemuxerInfo>& demuxers = sfe::Demuxer::getAvailableDemuxers();
		std::list<Demuxer::DemuxerInfo>::const_iterator it;
		
		std::cout << demuxers.size() << " demuxers available:" << std::endl;
		for (it = demuxers.begin(); it != demuxers.end();it++) {
			std::cout << "- " << it->name << " (" << it->description << ")" << std::endl;
		}
	}
	
	void dumpAvailableDecoders(void)
	{
		const std::list<Demuxer::DecoderInfo>& decoders = sfe::Demuxer::getAvailableDecoders();
		std::list<Demuxer::DecoderInfo>::const_iterator it;
		
		std::cout << decoders.size() << " decoders available:" << std::endl;
		for (it = decoders.begin(); it != decoders.end();it++) {
			std::cout << "- " << sfe::MediaTypeToString(it->type) << ": " << it->name << " (" << it->description << ")" << std::endl;
		}
	}
	
	std::string MediaTypeToString(MediaType type)
	{
		switch (type) {
			case MEDIA_TYPE_AUDIO:		return "audio";
			case MEDIA_TYPE_SUBTITLE:	return "subtitle";
			case MEDIA_TYPE_VIDEO:		return "video";
			case MEDIA_TYPE_UNKNOWN:	return "unknown";
		}
	}
}
