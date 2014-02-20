
/*
 *  Stream.cpp
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

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include "Stream.hpp"
#include "utils.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>

namespace sfe {
	Stream::Stream(AVStreamRef stream, DataSource& dataSource) :
	m_stream(NULL),
	m_dataSource(dataSource),
	m_codecCtx(NULL),
	m_codec(NULL),
	m_streamID(-1),
	m_packetList()
	{
		assert(stream);
		int err = 0;
		
		m_stream = stream;
		m_streamID = stream->index;
		m_codecCtx = stream->codec;
		
		// Get the video decoder
		m_codec = avcodec_find_decoder(m_codecCtx->codec_id);
		CHECK(m_codec, "Stream() - no decoder for codec " + std::string(avcodec_get_name(m_codecCtx->codec_id)));
		
		// Load the video codec
		err = avcodec_open2(m_codecCtx, m_codec, NULL);
		CHECK0(err, "Stream() - unable to load decoder for codec " + std::string(avcodec_get_name(m_codecCtx->codec_id)));
	}
	
	Stream::~Stream()
	{
		avcodec_close(m_codecCtx);
		
		AVPacketRef pkt;
		while (NULL != (pkt = popEncodedData()))
		{
			av_free_packet(pkt);
			av_free(pkt);
		}
	}
	
	void Stream::pushEncodedData(AVPacketRef packet)
	{
		assert(packet);
		m_packetList.push(packet);
	}
	
	AVPacketRef Stream::popEncodedData(void)
	{
		AVPacketRef result = NULL;
		
		if (m_packetList.size()) {
			result = m_packetList.front();
			m_packetList.pop();
		}
		
		return result;
	}
	
	bool Stream::needsMoreData(void) const
	{
		return m_packetList.size() < 10;
	}
}
