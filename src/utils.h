
/*
 *  utils.h
 *  SFE (SFML Extension) project
 *
 *  Copyright (C) 2010-2012 Soltic Lucas
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

#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <sstream>
#include <SFML/System.hpp>

#define ONCE(sequence)\
{ static bool __done = false; if (!__done) { { sequence; } __done = true; } }

#define ONCE_PSEC(sequence)\
{ static sf::Clock __timer; static bool __first = true; if (__first || __timer.GetElapsedTime() >= 1000) { { sequence; } __first = false; __timer.Reset(); }}

void PrintWithTime(const std::string& msg);

template <typename T>
std::string s(const T& v)
{
	std::ostringstream oss;
	oss << v;
	return oss.str();
}


#define EVALUATE_PORTION(portion)\
{\
static sf::Uint32 __time = 0;\
sf::Clock __eval_timer;\
{ portion; }\
__time += __eval_timer.getElapsedTime();\
ONCE_PSEC(\
		  std::cout << "time spent here : " << (__time / 10.f) << "% in one second" << std::endl;\
		  __time = 0;\
		  );\
}

#if 1

	#define SHOW_LOCKTIME(action) action

#else

	#define SHOW_LOCKTIME(action)\
	{\
	static sf::Clock __lock_sampler;\
	static unsigned __lock_time;\
	__lock_sampler.Reset();\
	{ action; }\
	__lock_time = __lock_sampler.GetElapsedTime();\
	if (__lock_time > 0) std::cout << "From " << __func__ << " line " << __LINE__ << ": locking took " << __lock_time << "ms\n";\
	}

#endif

#define UNIMPLEMENTED(class_name) {\
if (strlen(#class_name))\
std::cerr << "Warning: " << #class_name << "::" << __FUNCTION__ << "() called but is not implemented yet and therefore has no effect." << std::endl;\
else \
std::cerr << "Warning: " << __FUNCTION__ << "() called but is not implemented yet and therefore has no effect." << std::endl;\
}

#define EVAL_CHECKPOINT \
{\
static int __counter = 0;\
static sf::Clock __timer;\
if (__timer.GetElapsedTime() >= 1000)\
{\
	std::cout << "checkpoint passed " << __counter << " times this sec" << std::endl;\
	__counter = 0;\
	__timer.Reset();\
}\
__counter++;\
}

#endif

extern sf::Mutex __mtx;
extern void output_thread(void);
#define MT_COUT(sequence)\
{\
	sf::Lock __l(__mtx);\
	sf::Sleep(0);\
}
