
/*
 *  utils.cpp
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

#include "utils.hpp"
#include <ctime>
#include <iostream>

sf::Mutex __mtx;

static sf::Mutex printWithTimeMutex;
static sf::Clock globalTimer;

void printWithTime(const std::string& msg)
{
	sf::Lock l(printWithTimeMutex);
	printf("[%.3fs] %s\n", globalTimer.getElapsedTime().asSeconds(), msg.c_str());
	/*struct timeval tp;
	gettimeofday(&tp, NULL);
	
	tp.tv_sec %= 60;
	
	std::cout << tp.tv_sec << "." << tp.tv_usec << ": " << msg << std::endl;*/
}

void output_thread(void)
{
	//std::cout << "Thread " << (unsigned)pthread_self() % 1000 << ": ";
}
