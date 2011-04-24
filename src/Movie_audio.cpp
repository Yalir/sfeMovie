
/*
 *  Movie_audio.cpp
 *  SFE (SFML Extension) project
 *
 *  Copyright (C) 2010-2011 Soltic Lucas
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
	m_sampleRate(0)
	{
		
	}
	
	Movie_audio::~Movie_audio(void)
	{
		Close();
	}
	
	bool Movie_audio::Initialize(void)
	{
		int err;
		
		// Find the audio stream among the differents streams
		for (int i = 0; -1 == m_streamID && i < m_parent.GetAVFormatContext()->nb_streams; i++) 
		{ 
			if (m_parent.GetAVFormatContext()->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO) 
				m_streamID = i;
		}
		
		if (-1 == m_streamID)
			return false;
		
		// Get the audio codec
		m_codecCtx = m_parent.GetAVFormatContext()->streams[m_streamID]->codec;
		
		// Get the audio decoder (skip if there is no audio chanel)
		if (!m_codecCtx)
			return false;
		
		// If we have audio, go on! Find the decoder
		m_codec = avcodec_find_decoder(m_codecCtx->codec_id); 
		if (NULL == m_codec) 
		{
			std::cerr << "Movie_audio::Initialize() - could not find any audio decoder for this audio format" << std::endl;
			Close();
			return false; 
		} 
		
		// Load the audio codec
		err = avcodec_open(m_codecCtx, m_codec);
		if (err < 0)
		{
			std::cerr << "Movie_audio::Initialize() - unable to load the audio decoder for this audio format" << std::endl;
			Close();
			return false;
		}
		
		m_buffer = (sf::Int16 *)av_malloc(AUDIO_BUFSIZ);
		if (!m_buffer)
		{
			std::cerr << "Movie_audio::Initialize() - memory allocation error" << std::endl;
			Close();
			return false;
		}
		
		// Get some audio informations
		m_channelsCount = m_codecCtx->channels;
		m_sampleRate = m_codecCtx->sample_rate;
		
		// Initialize the sf::SoundStream
		sf::SoundStream::Initialize(m_channelsCount, m_sampleRate);
		
		return true;
	}
	
	void Movie_audio::Stop(void)
	{
		sf::SoundStream::Stop();
		
		if (av_seek_frame(m_parent.GetAVFormatContext(), m_streamID, 0, AVSEEK_FLAG_BACKWARD) < 0)
		{
			std::cerr << "Movie_audio::Stop() - av_seek_frame() error" << std::endl;
		}
		
		while (m_packetList.size()) {
			PopFrame();
		}
		
	}
	
	void Movie_audio::Close(void)
	{
		if (m_codecCtx)
			avcodec_close(m_codecCtx), m_codecCtx = NULL;
		
		m_codec = NULL;
		
		while (m_packetList.size())
			PopFrame();
		
		m_streamID = -1;
		
		if (m_buffer)
			av_free(m_buffer), m_buffer = NULL;
		
		m_channelsCount = 0;
		m_sampleRate = 0;
	}
	
	void Movie_audio::SetPlayingOffset(float time)
	{
		sf::SoundStream::Stop();
		
		// TODO: does not work yet
		AVRational tb = m_parent.GetAVFormatContext()->streams[m_streamID]->time_base;
		float ftb = (float)tb.num / tb.den;
		int64_t avTime = time * ftb;
		int res = av_seek_frame(m_parent.GetAVFormatContext(), m_streamID, avTime, AVSEEK_FLAG_BACKWARD);
		
		if (res < 0)
			std::cerr << "Movie_audio::SetPlayingOffset() - av_seek_frame() failed" << std::endl;
		else
		{
			while (m_packetList.size()) {
				PopFrame();
			}
			
			sf::SoundStream::Play();
		}
	}
	
	int Movie_audio::GetStreamID()
	{
		return m_streamID;
	}
	
	bool Movie_audio::ReadChunk(void)
	{
		if (m_parent.GetEofReached())
			return !m_parent.GetEofReached();
		
		// Read the movie file until we get an audio frame
		while (CurrentlyPendingDataLength() < AUDIO_BUFSIZ && !m_parent.GetEofReached())
			m_parent.ReadFrameAndQueue();
		
		return (CurrentlyPendingDataLength() != 0);
	}
		
	bool Movie_audio::HasPendingDecodableData(void)
	{
		sf::Lock l(m_packetListMutex);
		return !m_packetList.empty();
	}
	
	unsigned Movie_audio::CurrentlyPendingDataLength(void)
	{
		return m_pendingDataLength;
	}
	
	void Movie_audio::DecodeFrontFrame(Chunk& sfBuffer)
	{
		unsigned audioPacketOffset = 0;
		int res = 1;
		sfBuffer.Samples = NULL;
		sfBuffer.NbSamples = 0;
		
		// This buffer is used by sf::SoundStream and should therefore
		// not be destroyed before we're done with the previous chunk
		//if (m_buffer) av_free(m_buffer);
		//m_buffer = (sf::Int16 *)av_malloc(AUDIO_BUFSIZ);
		
		while (audioPacketOffset < m_sampleRate && res)
		{
			int frame_size = AUDIO_BUFSIZ;
			AVPacket *audioPacket = NULL;
			
			// Stop here if there is no frame to decode
			if (!HasPendingDecodableData())
			{
				if (!ReadChunk())
				{
					if (Movie::UsesDebugMessages())
						std::cerr << "Movie_audio::DecodeFrontFrame() - no frame currently available for decoding. Aborting decoding sequence." << std::endl;
					return;
				}
			}
			
			
			// Get the front audio packet
			audioPacket = FrontFrame();
			
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
					if (Movie::UsesDebugMessages())
					{
						ONCE(std::cerr << "Movie_audio::DecodeFrontFrame() - audio format for the current movie is not signed 16 bits and sfe::Movie does not support audio resampling yet" << std::endl);
					}
				}
				
				sfBuffer.Samples = m_buffer;
				sfBuffer.NbSamples = audioPacketOffset / sizeof(sf::Int16);
			}
			
			PopFrame();
		}
	}
		
	void Movie_audio::PushFrame(AVPacket *pkt)
	{
		sf::Lock l(m_packetListMutex);
		m_packetList.push(pkt);
		m_pendingDataLength += pkt->size;
	}
	
	void Movie_audio::PopFrame(void)
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
	
	AVPacket *Movie_audio::FrontFrame(void)
	{
		assert(!m_packetList.empty());
		
		sf::Lock l(m_packetListMutex);
		return m_packetList.front();
	}
	
	bool Movie_audio::OnGetData(Chunk& buffer)
    {
		bool flag = true;
        
		if (!HasPendingDecodableData())
			flag = ReadChunk();
		
		if (flag)
		{
			DecodeFrontFrame(buffer);
			
			if (!buffer.NbSamples)
				flag = false;
		}
		
        return flag;
    }

	
	void Movie_audio::OnSeek(float timeOffset)
	{
		
	}
	
} // namespace sfe

