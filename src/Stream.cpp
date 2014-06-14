
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
#include "Utilities.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>

namespace sfe {
	Stream::Stream(AVFormatContextRef formatCtx, AVStreamRef stream, DataSource& dataSource, Timer& timer) :
	m_formatCtx(formatCtx),
	m_stream(NULL),
	m_dataSource(dataSource),
	m_timer(timer),
	m_codecCtx(NULL),
	m_codec(NULL),
	m_streamID(-1),
	m_packetList(),
	m_status(Stopped),
	m_readerMutex()
	{
		CHECK(stream, "Stream::Stream() - invalid stream argument");
		int err = 0;
		
		m_stream = stream;
		m_streamID = stream->index;
		m_codecCtx = stream->codec;
		
		// Get the video decoder
		m_codec = avcodec_find_decoder(m_codecCtx->codec_id);
		CHECK(m_codec, "Stream() - no decoder for " + std::string(avcodec_get_name(m_codecCtx->codec_id)) + " codec");
		
		// Load the video codec
		err = avcodec_open2(m_codecCtx, m_codec, NULL);
		CHECK0(err, "Stream() - unable to load decoder for codec " + std::string(avcodec_get_name(m_codecCtx->codec_id)));
	}
	
	Stream::~Stream()
	{
		disconnect();
		flushBuffers();
		
		if (m_codecCtx)
			avcodec_close(m_codecCtx);
	}
	
	void Stream::connect()
	{
		m_timer.addObserver(*this);
	}
	
	void Stream::disconnect()
	{
		m_timer.removeObserver(*this);
	}
	
	void Stream::pushEncodedData(AVPacketRef packet)
	{
		CHECK(packet, "invalid argument");
		sf::Lock l(m_readerMutex);
		m_packetList.push_back(packet);
	}
	
	void Stream::prependEncodedData(AVPacketRef packet)
	{
		CHECK(packet, "invalid argument");
		sf::Lock l(m_readerMutex);
		m_packetList.push_front(packet);
	}
	
	AVPacketRef Stream::popEncodedData()
	{
		AVPacketRef result = NULL;
		sf::Lock l(m_readerMutex);
		
		if (!m_packetList.size()) {
			m_dataSource.requestMoreData(*this);
		}
		
		if (m_packetList.size()) {
			result = m_packetList.front();
			m_packetList.pop_front();
		} else {
			if (m_codecCtx->codec->capabilities & CODEC_CAP_DELAY) {
				AVPacketRef flushPacket = (AVPacketRef)av_malloc(sizeof(*flushPacket));
				av_init_packet(flushPacket);
				flushPacket->data = NULL;
				flushPacket->size = 0;
				result = flushPacket;
				
				sfeLogDebug("Sending flush packet: " + MediaTypeToString(getStreamKind()));
			}
		}
		
		return result;
	}
	
	void Stream::flushBuffers()
	{
		sf::Lock l(m_readerMutex);
		if (getStatus() == Playing) {
			sfeLogWarning("packets flushed while the stream is still playing");
		}
		
		avcodec_flush_buffers(m_codecCtx);
		discardAllEncodedData();
		
		sfeLogDebug("Flushed " + MediaTypeToString(getStreamKind()) + " stream!");
	}
	
	bool Stream::needsMoreData() const
	{
		return m_packetList.size() < 10;
	}
	
	MediaType Stream::getStreamKind() const
	{
		return MEDIA_TYPE_UNKNOWN;
	}
	
	Status Stream::getStatus() const
	{
		return m_status;
	}
	
	sf::Time Stream::computePosition()
	{
		if (!m_packetList.size()) {
			m_dataSource.requestMoreData(*this);
		}
		
		if (!m_packetList.size()) {
			return sf::Time();
		} else {
			AVPacketRef packet = *m_packetList.begin();
			CHECK(packet, "internal inconcistency");
			
			int64_t timestamp = -424242;
			
			if (packet->dts != AV_NOPTS_VALUE) {
				timestamp = packet->dts;
			} else if (packet->pts != AV_NOPTS_VALUE) {
				int64_t startTime = m_stream->start_time != AV_NOPTS_VALUE ? m_stream->start_time : 0;
				timestamp = packet->pts - startTime;
			}
			
			return sf::seconds(timestamp * av_q2d(m_stream->time_base));
		}
	}
	
	void Stream::setStatus(Status status)
	{
		m_status = status;
	}
	
	void Stream::discardAllEncodedData()
	{
		AVPacketRef pkt = NULL;
		
		while (m_packetList.size()) {
			pkt = m_packetList.front();
			m_packetList.pop_front();
			
			av_free_packet(pkt);
			av_free(pkt);
		}
	}
	
	void Stream::didPlay(const Timer& timer, Status previousStatus)
	{
		setStatus(Playing);
	}
	
	void Stream::didPause(const Timer& timer, Status previousStatus)
	{
		setStatus(Paused);
	}
	
	void Stream::didStop(const Timer& timer, Status previousStatus)
	{
		setStatus(Stopped);
	}
	
	void Stream::willSeek(const Timer& timer, sf::Time position)
	{
	}
	
	void Stream::didSeek(const Timer& timer, sf::Time position)
	{
		flushBuffers();
	}
}
