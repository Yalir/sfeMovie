
/*
 *  utils.h
 *  SFE (SFML Extension) project
 *
 *  Copyright (C) 2010-2011 Soltic Lucas
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
#include <SFML/System/Clock.hpp>

#define ONCE(sequence)\
{ static bool __done = false; if (!__done) { { sequence; } __done = true; } }

#define ONCE_PSEC(sequence)\
{ static sf::Clock __timer; static bool __first = true; if (__first || __timer.GetElapsedTime() > 1.f) { { sequence; } __first = false; __timer.Reset(); }}

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
static float __time = 0.f;\
sf::Clock __eval_timer;\
{ portion; }\
__time += __eval_timer.GetElapsedTime();\
ONCE_PSEC(\
		  std::cout << "time spent here : " << __time * 100 << "% in one second" << std::endl;\
		  __time = 0.f;\
		  );\
}

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
if (__timer.GetElapsedTime() > 1)\
{\
	std::cout << "checkpoint passed " << __counter << " times this sec" << std::endl;\
	__counter = 0;\
	__timer.Reset();\
}\
__counter++;\
}

#endif
