
/*
 *  Threads.hpp
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

#ifndef SFEMOVIE_THREADS_HPP
#define SFEMOVIE_THREADS_HPP

#include <string>

namespace sfe {
	namespace Threads {
		/** Set the current thread name to the given name
		 *
		 * This function is thread safe.
		 *
		 * @param name the name to give to the current thread
		 */
		void nameCurrentThread(const std::string& name);
		
		/** Get the current thread name
		 *
		 * This function is thread safe.
		 *
		 * @return the name of the current thread, or "unnamed thread" if it was not assigned a name
		 */
		const std::string& currentThreadName(void);
	}
}

#endif
