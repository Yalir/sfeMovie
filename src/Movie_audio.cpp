
/*
 *  Movie_audio.cpp
 *  SFE (SFML Extension) project
 *
 *  Copyright (C) 2010-2012 Lucas Soltic
 *  soltic.lucas@gmail.com
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

#include "Movie_audio.h"
#include "Movie.h"
#include <iostream>
#include <cassert>
#include "utils.h"

#define AUDIO_BUFSIZ AVCODEC_MAX_AUDIO_FRAME_SIZE // 192000 bytes, 1 second of 48kHz 32bit audio

namespace sfe {
	
	Movie_audio::Movie_audio(Movie& parent) :
	m_parent(parent),
	m_codecCtx(NULL),
	m_codec(NULL),
	m_packetList(),
	m_packetListMutex(),
	m_streamID(-1),
	m_buffer(NULL),
	m_pendingDataLength(0),
	m_channelsCount(0),
	m_sampleRate(0),
	m_isStarving(false)
	{
		
	}
	
	Movie_audio::~Movie_audio(void)
	{
	}
	
	bool Movie_audio::initialize(void)
	{
		int err;
		
		// Find the audio stream among the differents streams
		for (int i = 0; -1 == m_streamID && i < m_parent.getAVFormatContext()->nb_streams; i++) 
		{ 
			if (m_parent.getAVFormatContext()->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) 
				m_streamID = i;
		}
		
		if (-1 == m_streamID)
			return false;
		
		// Get the audio codec
		m_codecCtx = m_parent.getAVFormatContext()->streams[m_streamID]->codec;
		
		// Get the audio decoder (skip if there is no audio chanel)
		if (!m_codecCtx)
			return false;
		
		// If we have audio, go on! Find the decoder
		m_codec = avcodec_find_decoder(m_codecCtx->codec_id); 
		if (NULL == m_codec) 
		{
			std::cerr << "Movie_audio::Initialize() - could not find any audio decoder for this audio format" << std::endl;
			close();
			return false; 
		} 
		
		// Load the audio codec
		err = avcodec_open2(m_codecCtx, m_codec, NULL);
		if (err < 0)
		{
			std::cerr << "Movie_audio::Initialize() - unable to load the audio decoder for this audio format" << std::endl;
			close();
			return false;
		}
		
		m_buffer = (sf::Int16 *)av_malloc(AUDIO_BUFSIZ);
		if (!m_buffer)
		{
			std::cerr << "Movie_audio::Initialize() - memory allocation error" << std::endl;
			close();
			return false;
		}
		
		// Get some audio informations
		m_channelsCount = m_codecCtx->channels;
		m_sampleRate = m_codecCtx->sample_rate;
		
		// Initialize the sf::SoundStream
		sf::SoundStream::initialize(m_channelsCount, m_sampleRate);
		
		return true;
	}
	
	void Movie_audio::stop(void)
	{
		sf::SoundStream::stop();
		
		if (av_seek_frame(m_parent.getAVFormatContext(), m_streamID, 0, AVSEEK_FLAG_BACKWARD) < 0)
		{
			std::cerr << "Movie_audio::Stop() - av_seek_frame() error" << std::endl;
		}
		
		while (m_packetList.size()) {
			popFrame();
		}
		
		m_isStarving = false;
	}
	
	void Movie_audio::close(void)
	{
		if (m_codecCtx)
			avcodec_close(m_codecCtx), m_codecCtx = NULL;
		
		m_codec = NULL;
		
		while (m_packetList.size())
			popFrame();
		
		m_streamID = -1;
		
		if (m_buffer)
			av_free(m_buffer), m_buffer = NULL;
		
		m_channelsCount = 0;
		m_sampleRate = 0;
		m_isStarving = false;
	}
	
	void Movie_audio::setPlayingOffset(sf::Time time)
	{
		sf::SoundStream::stop();
		
		// TODO: does not work yet
		// TODO: apply float -> Uin32 change
		AVRational tb = m_parent.getAVFormatContext()->streams[m_streamID]->time_base;
		float ftb = (float)tb.num / tb.den;
		int64_t avTime = time.asMilliseconds() * ftb;
		int res = av_seek_frame(m_parent.getAVFormatContext(), m_streamID, avTime, AVSEEK_FLAG_BACKWARD);
		
		if (res < 0)
			std::cerr << "Movie_audio::SetPlayingOffset() - av_seek_frame() failed" << std::endl;
		else
		{
			while (m_packetList.size()) {
				popFrame();
			}
			
			sf::SoundStream::play();
		}
	}
	
	int Movie_audio::getStreamID()
	{
		return m_streamID;
	}
	
	bool Movie_audio::isStarving(void)
	{
		return m_isStarving;
	}
	
	bool Movie_audio::readChunk(void)
	{
		if (m_parent.getEofReached())
			return !m_parent.getEofReached();
		
		// Read the movie file until we get an audio frame
		while (currentlyPendingDataLength() < AUDIO_BUFSIZ && !m_parent.getEofReached())
			m_parent.readFrameAndQueue();
		
		return (currentlyPendingDataLength() != 0);
	}
		
	bool Movie_audio::hasPendingDecodableData(void)
	{
		sf::Lock l(m_packetListMutex);
		return !m_packetList.empty();
	}
	
	unsigned Movie_audio::currentlyPendingDataLength(void)
	{
		return m_pendingDataLength;
	}
	
	void Movie_audio::decodeFrontFrame(Chunk& sfBuffer)
	{
		unsigned audioPacketOffset = 0;
		int res = 1;
		sfBuffer.samples = NULL;
		sfBuffer.sampleCount = 0;
		
		// This buffer is used by sf::SoundStream and should therefore
		// not be destroyed before we're done with the previous chunk
		//if (m_buffer) av_free(m_buffer);
		//m_buffer = (sf::Int16 *)av_malloc(AUDIO_BUFSIZ);
		
		while (audioPacketOffset < m_sampleRate && res)
		{
			int frame_size = AUDIO_BUFSIZ;
			AVPacket *audioPacket = NULL;
			
			// Stop here if there is no frame to decode
			if (!hasPendingDecodableData())
			{
				if (!readChunk())
				{
					if (Movie::usesDebugMessages())
						std::cerr << "Movie_audio::DecodeFrontFrame() - no frame currently available for decoding. Aborting decoding sequence." << std::endl;
					return;
				}
			}
			
			
			// Get the front audio packet
			audioPacket = frontFrame();
			
			// Decode it
			res = avcodec_decode_audio3(m_codecCtx,
										(sf::Int16 *)((char *)m_buffer + audioPacketOffset),
										&frame_size, audioPacket);
			
			if (res < 0)
			{
				std::cerr << "Movie_audio::DecodeFrontFrame() - an error occured while decoding the audio frame" << std::endl;
			}
			else
			{
				audioPacketOffset += frame_size;

				if (m_codecCtx->sample_fmt != SAMPLE_FMT_S16)
				{
					// Never happened to me for now, which is fine
					if (Movie::usesDebugMessages())
					{
						ONCE(std::cerr << "Movie_audio::DecodeFrontFrame() - audio format for the current movie is not signed 16 bits and sfe::Movie does not support audio resampling yet" << std::endl);
					}
				}
				
				sfBuffer.samples = m_buffer;
				sfBuffer.sampleCount = audioPacketOffset / sizeof(sf::Int16);
			}
			
			popFrame();
		}
	}
		
	void Movie_audio::pushFrame(AVPacket *pkt)
	{
		sf::Lock l(m_packetListMutex);
		m_packetList.push(pkt);
		m_pendingDataLength += pkt->size;
	}
	
	void Movie_audio::popFrame(void)
	{
		sf::Lock l(m_packetListMutex);
		
		if (!m_packetList.empty())
		{
			AVPacket *pkt = m_packetList.front();
			m_pendingDataLength -= pkt->size;
			m_packetList.pop();
			av_free_packet(pkt);
			av_free(pkt);
		}
	}
	
	AVPacket *Movie_audio::frontFrame(void)
	{
		assert(!m_packetList.empty());
		
		sf::Lock l(m_packetListMutex);
		return m_packetList.front();
	}
	
	bool Movie_audio::onGetData(Chunk& buffer)
    {
		bool flag = true;
        
		if (!hasPendingDecodableData())
			flag = readChunk();
		
		if (flag)
		{
			decodeFrontFrame(buffer);
			
			if (!buffer.sampleCount)
			{
				flag = false;
			}
		}
		
		if (!flag)
		{
			m_isStarving = true;
			m_parent.starvation();
		}
		
        return flag;
    }

	
	void Movie_audio::onSeek(sf::Time timeOffset)
	{
		
	}
	
} // namespace sfe

