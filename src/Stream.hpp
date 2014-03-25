
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
#include <list>

namespace sfe {
	
	enum MediaType {
		MEDIA_TYPE_AUDIO,
		MEDIA_TYPE_SUBTITLE,
		MEDIA_TYPE_VIDEO,
		MEDIA_TYPE_UNKNOWN
	};
	
	class Stream : public Timer::Observer {
	public:
		/** The stream's status
		 */
		enum Status {
			Stopped,
			Paused,
			Playing
		};
		
		struct DataSource {
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
		Stream(AVFormatContextRef formatCtx, AVStreamRef stream, DataSource& dataSource, Timer& timer);
		
		/** Default destructor
		 */
		virtual ~Stream(void);
		
		/** Connect this stream against the reference timer to receive playback events; this allows this
		 * stream to be played
		 */
		void connect(void);
		
		/** Disconnect this stream from the reference timer ; this disables this stream
		 */
		void disconnect(void);
		
		/** Called by the demuxer to provide the stream with encoded data
		 *
		 * @return packet the encoded data usable by this stream
		 */
		virtual void pushEncodedData(AVPacketRef packet);
		
		/** Reinsert an AVPacket at the beginning of the queue
		 *
		 * This is used for packets that contain several frames, but whose next frames
		 * cannot be decoded yet. These packets are repushed to be decoded when possible.
		 *
		 * @param packet the packet to re-insert at the beginning of the queue
		 */
		virtual void prependEncodedData(AVPacketRef packet);
		
		/** Return the oldest encoded data that was pushed to this stream
		 *
		 * If no packet is stored when this method is called, it will ask the
		 * data source to feed this stream first
		 *
		 * @return the oldest encoded data, or null if no data could be read from the media
		 */
		virtual AVPacketRef popEncodedData(void);
		
		/** Empty the encoded data queue and destroy all the packets
		 */
		virtual void discardAllEncodedData(void);
		
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
		
		/** Give the stream's status
		 *
		 * @return The stream's status (Playing, Paused or Stopped)
		 */
		Status getStatus(void) const;
		
		/** Update the current stream's status and eventually decode frames
		 */
		virtual void update(void) = 0;
	protected:
		// Timer::Observer interface
		virtual void didPlay(const Timer& timer, Timer::Status previousStatus) = 0;
		virtual void didPause(const Timer& timer, Timer::Status previousStatus) = 0;
		virtual void didStop(const Timer& timer, Timer::Status previousStatus) = 0;
		
		void setStatus(Status status);
		
		AVFormatContextRef m_formatCtx;
		AVStreamRef m_stream;
		DataSource& m_dataSource;
		Timer& m_timer;
		AVCodecContextRef m_codecCtx;
		AVCodecRef m_codec;
		int m_streamID;
		std::list <AVPacketRef> m_packetList;
		Status m_status;
	};
}

#endif
