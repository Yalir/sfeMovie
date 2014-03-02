
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
#include "Timer.hpp"
#include <map>
#include <string>
#include <set>
#include <utility>

namespace sfe {
	class Demuxer : public Stream::DataSource {
	public:
		/** Return a list containing the names of all the decoders included
		 * in this sfeMovie build
		 */
		static const std::set<std::pair<std::string, MediaType> >& getAvailableDecoders(void);
		
		/** Default constructor
		 *
		 * Open a media file and find its streams
		 */
		Demuxer(const std::string& sourceFile, Timer& timer);
		
		/** Default destructor
		 */
		~Demuxer(void);
		
		/** Return a list of the streams found in the media
		 * The map key is the index of the stream
		 *
		 * @return the list of streams
		 */
		const std::map<int, Stream*>& getStreams(void) const;
		
		/** Read encoded data from the media and makes sure that the given stream
		 * has enough data
		 *
		 * @param stream The stream to feed
		 */
		void feedStream(Stream& stream);
		
		/** Tell whether the demuxer has reached the end of the file and can no more feed the streams
		 *
		 * @return whether the end of the media file has been reached
		 */
		bool didReachEndOfFile(void) const;
		
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
		
		virtual void requestMoreData(Stream& starvingStream);
		
		AVFormatContextRef m_avFormatCtx;
		bool m_eofReached;
		std::map<int, Stream*> m_streams;
		std::map<int, std::string> m_ignoredStreams;
		sf::Mutex m_synchronized;
		
		static std::set<std::pair<std::string, MediaType> > g_availableDecoders;
	};
}

#endif
