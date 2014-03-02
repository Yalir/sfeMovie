
/*
 *  Threads.cpp
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

#include "Threads.hpp"
#include <SFML/System.hpp>
#include <boost/thread.hpp>
#include <map>

namespace sfe {
	namespace Threads {
		static std::map<boost::thread::id, std::string> g_threadNames;
		static sf::Mutex g_synchronized;
		static const std::string g_unnamedThread = "unnamed thread";
		
		void nameCurrentThread(const std::string& name)
		{
			sf::Lock l(g_synchronized);
			g_threadNames[boost::this_thread::get_id()] = name;
		}
		
		const std::string& currentThreadName(void)
		{
			sf::Lock l(g_synchronized);
			std::map<boost::thread::id, std::string>::iterator it = g_threadNames.find(boost::this_thread::get_id());
			
			if (it != g_threadNames.end()) {
				return it->second;
			} else {
				return g_unnamedThread;
			}
		}
	}
}
