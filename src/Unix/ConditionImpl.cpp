
/*
 *  ConditionImpl.cpp (Unix)
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


#include "ConditionImpl.h"
#include "utils.h"
#include <iostream>
using namespace std;

ConditionImpl::ConditionImpl(int var) :
m_isValid(true),
m_conditionnedVar(var),
m_cond(),
m_mutex()
{
	if (0 != pthread_cond_init(&m_cond, NULL))
		cerr << "pthread_cond_init() error\n";
	
	if (0 != pthread_mutex_init(&m_mutex, NULL))
		cerr << "pthread_mutex_init() error\n";
}


ConditionImpl::~ConditionImpl(void)
{
	if (0 != pthread_mutex_destroy(&m_mutex))
		cerr << "pthread_cond_destroy() error\n";
	
	if (0 != pthread_cond_destroy(&m_cond))
		cerr << "pthread_cond_destroy() error\n";
}

bool ConditionImpl::waitAndRetain(int value)
{
	pthread_mutex_lock(&m_mutex);
	
	/*if (m_conditionnedVar == value && m_isValid)
	{
		MT_COUT(std::cout << "cond " << (unsigned)this%1000 << " DIRECTLY validated\n");
	}*/
	
	while (m_conditionnedVar != value && m_isValid)
	{
		//MT_COUT(std::cout << "will wait cond " << (unsigned)this%1000 << " (expected " << value << ")\n");
		pthread_cond_wait(&m_cond, &m_mutex);
	}
	
	if (m_isValid)
	{
		//MT_COUT(std::cout << "cond " << (unsigned)this%1000 << " validated\n");
		return true;
	}
	else
	{
		//MT_COUT(std::cout << "invalid condition\n");
		pthread_mutex_unlock(&m_mutex);
		return false;
	}
}

void ConditionImpl::release(int value)
{
	m_conditionnedVar = value;
	pthread_mutex_unlock(&m_mutex);
	
	signal();
}

void ConditionImpl::setValue(int value)
{
	// Make sure the Condition's value is not modified while retained
	pthread_mutex_lock(&m_mutex);
	//MT_COUT(std::cout << "cond " << (unsigned)this%1000 << " set to " << value << std::endl);
	m_conditionnedVar = value;
	pthread_mutex_unlock(&m_mutex);
	
	signal();
}


void ConditionImpl::signal(void)
{
	pthread_cond_signal(&m_cond);
}


void ConditionImpl::invalidate(void)
{
	if (m_isValid)
	{
		m_isValid = false;
		signal();
	}
}


void ConditionImpl::restore(void)
{
	if (!m_isValid)
	{
		m_isValid = true;
	}
}

