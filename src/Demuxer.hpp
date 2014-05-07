
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

#ifndef SFEMOVIE_DEMUXER_HPP
#define SFEMOVIE_DEMUXER_HPP

#include <SFML/System.hpp>
#include "Macros.hpp"
#include "Stream.hpp"
#include "AudioStream.hpp"
#include "VideoStream.hpp"
#include "Timer.hpp"
#include <map>
#include <string>
#include <set>
#include <list>
#include <utility>

namespace sfe {
	class Demuxer : public Stream::DataSource, public Timer::Observer {
	public:
		/** Describes a demuxer
		 *
		 * Ie. an audio/video container format parser such as avi, mov, mkv, ogv... parsers
		 */
		struct DemuxerInfo {
			std::string name;
			std::string description;
		};
		
		/** Describes a decoder
		 *
		 * Ie. an audio/video/subtitle stream decoder for h.264, theora, vp9, mp3, pcm, srt... streams
		 */
		struct DecoderInfo {
			std::string name;
			std::string description;
			MediaType type;
		};
		
		/** Return a list containing the names of all the demuxers (ie. container parsers) included
		 * in this sfeMovie build
		 */
		static const std::list<DemuxerInfo>& getAvailableDemuxers(void);
		
		/** Return a list containing the names of all the decoders included
		 * in this sfeMovie build
		 */
		static const std::list<DecoderInfo>& getAvailableDecoders(void);
		
		/** Default constructor
		 *
		 * Open a media file and find its streams
		 *
		 * @param sourceFile the path of the media to open and play
		 * @param timer the timer with which the media streams will be synchronized
		 * @param videoDelegate the delegate that will handle the images produced by the VideoStreams
		 */
		Demuxer(const std::string& sourceFile, Timer& timer, VideoStreamDelegate& videoDelegate);
		
		/** Default destructor
		 */
		virtual ~Demuxer(void);
		
		/** Return a list of the streams found in the media
		 * The map key is the index of the stream
		 *
		 * @return the list of streams
		 */
		const std::map<int, Stream*>& getStreams(void) const;
		
		/** Return a set containing all the streams found in the media that match the given type
		 *
		 * @param the media type against which the returned streams should be filtered
		 * @return the audio streams
		 */
		std::set<Stream*> getStreamsOfType(MediaType type) const;
		
		/** Enable the given audio stream and connect it to the reference timer
		 * 
		 * If another stream of the same kind is already enabled, it is first disabled and disconnected
		 * so that only one stream of the same kind can be enabled at the same time.
		 *
		 * @param stream the audio stream to enable and connect for playing, or NULL to disable audio
		 */
		void selectAudioStream(AudioStream* stream);
		
		/** Enable the first found audio stream, if it exists
		 * 
		 * @see selectAudioStream
		 */
		void selectFirstAudioStream(void);
		
		/** Get the currently selected audio stream, if there's one
		 *
		 * @return the currently selected audio stream, or NULL if there's none
		 */
		AudioStream* getSelectedAudioStream(void) const;
		
		/** Enable the given video stream and connect it to the reference timer
		 *
		 * If another stream of the same kind is already enabled, it is first disabled and disconnected
		 * so that only one stream of the same kind can be enabled at the same time.
		 *
		 * @param stream the video stream to enable and connect for playing, or NULL to disable video
		 */
		void selectVideoStream(VideoStream* stream);
		
		/** Enable the first found video stream, if it exists
		 *
		 * @see selectAudioStream
		 */
		void selectFirstVideoStream(void);
		
		/** Get the currently selected video stream, if there's one
		 *
		 * @return the currently selected video stream, or NULL if there's none
		 */
		VideoStream* getSelectedVideoStream(void) const;
		
		/** Read encoded data from the media and makes sure that the given stream
		 * has enough data
		 *
		 * @param stream The stream to feed
		 */
		void feedStream(Stream& stream);
		
		/** Update the media status and eventually decode frames
		 */
		void update(void);
		
		/** Tell whether the demuxer has reached the end of the file and can no more feed the streams
		 *
		 * @return whether the end of the media file has been reached
		 */
		bool didReachEndOfFile(void) const;
		
		/** Give the media duration
		 *
		 * @return the media duration
		 */
		sf::Time getDuration(void) const;
		
	private:
		/** Read a encoded packet from the media file
		 *
		 * You're responsible for freeing the returned packet
		 *
		 * @return the read packet, or NULL if the end of file has been reached
		 */
		AVPacketRef readPacket(void);
		
		/** Distribute the given packet to the correct stream
		 *
		 * If the packet doesn't match any known stream, nothing is done
		 *
		 * @param packet the packet to distribute
		 * @return true if the packet could be distributed, false otherwise
		 */
		bool distributePacket(AVPacketRef packet);
		
		/** Try to extract the media duration from the given stream
		 */
		void extractDurationFromStream(AVStreamRef stream);
		
		// Data source interface
		void requestMoreData(Stream& starvingStream);
		void resetEndOfFileStatus(void);
		
		// Timer interface
		void willSeek(const Timer& timer, sf::Time position);
		
		AVFormatContextRef m_formatCtx;
		bool m_eofReached;
		std::map<int, Stream*> m_streams;
		std::map<int, std::string> m_ignoredStreams;
		sf::Mutex m_synchronized;
		Timer& m_timer;
		Stream* m_connectedAudioStream;
		Stream* m_connectedVideoStream;
		sf::Time m_duration;
		
		static std::list<DemuxerInfo> g_availableDemuxers;
		static std::list<DecoderInfo> g_availableDecoders;
	};
}

#endif
