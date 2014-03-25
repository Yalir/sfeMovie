
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

#ifndef SFEMOVIE_LOG_HPP
#define SFEMOVIE_LOG_HPP

#include <string>
#include <sstream>

#define sfeLogDebug(message) sfe::Log::debug(__FILE__, std::string(":") + sfe::s(__LINE__) + ": " + std::string(__func__) + "()" + " - " + message)
#define sfeLogWarning(message) sfe::Log::warning(__FILE__, std::string(":") + sfe::s(__LINE__) + ": " + std::string(__func__) + "()" + " - " + message)
#define sfeLogError(message) sfe::Log::error(__FILE__, std::string(":") + sfe::s(__LINE__) + ": " + std::string(__func__) + "()" + " - " + message)

namespace sfe {
	namespace Log {
		enum Mask {
			EmptyMask = 0,
			DebugMask = 1 << 0,
			WarningMask = 1 << 1,
			ErrorMask = 1 << 2
		};
		
		/** Log only the messages allowed by the given @a mask
		 *
		 * eg:
		 * setMask(DebugMask) to allow debug messages only
		 * setMask(0) to quiet logging
		 *
		 * @param mask the kind of messages that should be logged
		 */
		void setMask(int mask);
		
		/** Log a debug @a message if the currently set mask allows it
		 *
		 * @param message the debug message to log
		 */
		void debug(const std::string& file, const std::string& message);
		
		/** Log a warning @a message if the currently set mask allows it
		 *
		 * @param message the warning message to log
		 */
		void warning(const std::string& file, const std::string& message);
		
		/** Log an error @a message if the currently set mask allows it
		 *
		 * @param message the error message to log
		 */
		void error(const std::string& file, const std::string& message);
	}
	
	template <typename T>
	std::string s(const T& obj)
	{
		std::ostringstream ss;
		ss << obj;
		return ss.str();
	}
};

#endif
