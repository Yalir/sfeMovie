
/*
 *  VideoStream.cpp
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

extern "C" {
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
	#include <libswscale/swscale.h>
}

#include "VideoStream.hpp"

namespace sfe {
	VideoStream::VideoStream(AVStreamRef stream, DataSource& dataSource, Timer& timer) :
	Stream(stream, dataSource, timer)
	{
		
	}
	
	VideoStream::~VideoStream(void)
	{
		
	}
	
	void VideoStream::play(void)
	{
		
	}
	
	void VideoStream::pause(void)
	{
		
	}
	
	void VideoStream::stop(void)
	{
		
	}
	
	VideoStream::Kind VideoStream::getStreamKind(void) const
	{
		return VIDEO_STREAM;
	}
}
