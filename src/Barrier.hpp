
/*
 *  Barrier.hpp
 *  sfeMovie project
 *
 *  Copyright (C) 2010-2012 Lucas Soltic
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

#ifndef BARRIER_HPP
#define BARRIER_HPP

#include "Condition.hpp"
#include <SFML/System/Mutex.hpp>

namespace sfe {
	
	class Barrier {
	public:
		/** @brief Default constructor
		 * Initializes a new Barrier object that should block waiters until
		 * @a count threads are waiting
		 */
		Barrier(unsigned int count = 1);
		
		/** @brief Default destructor
		 */
		~Barrier(void);
		
		/** @brief Wait until @a count threads are waiting against this barrier
		 */
		void wait(void);
		
	private:
		sf::Mutex m_mutex;
		unsigned m_count;
		unsigned m_waitingCount;
		Condition m_cond;
	};
	
} // namespace sfe


#endif
