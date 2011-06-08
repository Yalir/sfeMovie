
/*
 *  Movie_audio.h
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


#ifndef MOVIE_AUDIO_H
#define MOVIE_AUDIO_H

extern "C" 
{ 
#include <libavformat/avformat.h> 
#include <libavcodec/avcodec.h> 
#include <libswscale/swscale.h>
}
#include <queue>
#include <SFML/System.hpp>
#include <SFML/Audio.hpp>

namespace sfe {
	class Movie;
	class Movie_audio : private sf::SoundStream {
	public:
		Movie_audio(Movie& parent);
		~Movie_audio(void);
		
		// -------------------------- Audio methods ----------------------------
		bool Initialize(void);
		void Stop(void);
		void Close(void);
		
		using sf::SoundStream::Play;
		using sf::SoundStream::Pause;
		using sf::SoundStream::SetVolume;
		using sf::SoundStream::GetVolume;
		using sf::SoundStream::GetSampleRate;
		using sf::SoundStream::GetChannelsCount;
		//using sf::SoundStream::SetPlayingOffset;
		using sf::SoundStream::GetPlayingOffset;
		
		void SetPlayingOffset(sf::Uint32 time);
		
		int GetStreamID();
		bool IsStarving(void);
		
		bool ReadChunk(void);
		bool HasPendingDecodableData(void);
		unsigned CurrentlyPendingDataLength(void);
		void DecodeFrontFrame(Chunk& sfBuffer);
		void PushFrame(AVPacket *pkt);
		void PopFrame(void);
		AVPacket *FrontFrame(void);
		
		bool OnGetData(Chunk& Data);
		void OnSeek(sf::Uint32 timeOffset);
		
	private:
		// ------------------------- Audio attributes --------------------------
		Movie& m_parent;
		
		// FFmpeg stuff
		AVCodecContext *m_codecCtx; 
		AVCodec *m_codec;
		std::queue <AVPacket *> m_packetList;

		sf::Mutex m_packetListMutex;
		int m_streamID;
		sf::Int16 *m_buffer; // Buffer used to store the current audio data chunk
		unsigned m_pendingDataLength;
		
		unsigned m_channelsCount;
		unsigned m_sampleRate;
		bool m_isStarving;
	}; // class Movie_audio
} // namespace sfe

#endif
