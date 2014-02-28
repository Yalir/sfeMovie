
/*
 *  Stream.hpp
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

#ifndef SFEMOVIE_STREAM_HPP
#define SFEMOVIE_STREAM_HPP

#include "Macros.hpp"
#include "Timer.hpp"
#include <queue>

namespace sfe {
	
	enum MediaType {
		MEDIA_TYPE_AUDIO,
		MEDIA_TYPE_SUBTITLE,
		MEDIA_TYPE_VIDEO,
		MEDIA_TYPE_UNKNOWN
	};
	
	class Stream : public Timer::Observer {
	public:
		class DataSource {
		public:
			virtual void requestMoreData(Stream& starvingStream) = 0;
		};
		
		/** Create a stream from the given FFmpeg stream
		 * 
		 * At the end of the constructor, the stream is guaranteed
		 * to have all of its fields set and the decoder loaded
		 *
		 * @param stream the FFmpeg stream
		 * @param dataSource the encoded data provider for this stream
		 */
		Stream(AVStreamRef stream, DataSource& dataSource, Timer& timer);
		
		/** Default destructor
		 */
		virtual ~Stream(void);
		
		/** Called by the demuxer to provide the stream with encoded data
		 *
		 * @return packet the encoded data usable by this stream
		 */
		virtual void pushEncodedData(AVPacketRef packet);
		
		/** Return the oldest encoded data that was pushed to this stream
		 *
		 * If no packet is stored when this method is called, it will ask the
		 * data source to feed this stream first
		 *
		 * @return the oldest encoded data, or null if no data could be read from the media
		 */
		virtual AVPacketRef popEncodedData(void);
		
		/** Used by the demuxer to know if this stream should be fed with more data
		 *
		 * The default implementation returns true if the packet list contains less than 10 packets
		 *
		 * @return true if the demuxer should give more data to this stream, false otherwise
		 */
		virtual bool needsMoreData(void) const;
		
		/** Get the stream kind (either audio, video or subtitle stream)
		 *
		 * @return the kind of stream represented by this stream
		 */
		virtual MediaType getStreamKind(void) const = 0;
		
	protected:
		// Timer::Observer interface
		virtual void didPlay(const Timer& timer, Timer::Status previousStatus);
		virtual void didPause(const Timer& timer, Timer::Status previousStatus);
		virtual void didStop(const Timer& timer, Timer::Status previousStatus);
		
		AVStreamRef m_stream;
		DataSource& m_dataSource;
		Timer& m_timer;
		AVCodecContextRef m_codecCtx;
		AVCodecRef m_codec;
		int m_streamID;
		std::queue <AVPacketRef> m_packetList;
	};
}

#endif
