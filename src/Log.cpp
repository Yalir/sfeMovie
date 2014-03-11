
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

#include "Log.hpp"
#include <iostream>
#include <SFML/System.hpp>

namespace sfe {
	namespace Log {
		static int g_logMask = WarningMask;
		static sf::Mutex g_synchronized;
		
		void setMask(int mask)
		{
			sf::Lock l(g_synchronized);
			g_logMask = mask;
		}
		
		void debug(const std::string& message)
		{
			sf::Lock l(g_synchronized);
			
			if (g_logMask & DebugMask) {
				std::cerr << "Debug: " << message << std::endl;
			}
		}
		
		void warning(const std::string& message)
		{
			sf::Lock l(g_synchronized);
			
			if (g_logMask & WarningMask) {
				std::cerr << "Warning: " << message << std::endl;
			}
		}
	}
}
