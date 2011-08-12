
/*
 *  ConditionImpl.cpp (Win32)
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
#include <iostream>

namespace sfe {

ConditionImpl::ConditionImpl(int var) :
m_isValid(true),
m_conditionnedVar(var),
m_mutex()
{
	m_cond = CreateEvent(NULL, FALSE, FALSE, NULL);
	
	if (m_cond == NULL)
		std::cerr << "ConditionImpl() - CreateEvent() error\n";
}

ConditionImpl::~ConditionImpl(void)
{
	CloseHandle(m_cond);
}

bool ConditionImpl::waitAndRetain(int value)
{
	m_mutex.Lock();
	
	while (m_conditionnedVar != value && m_isValid)
	{
		m_mutex.Unlock();
		WaitForSingleObject(m_cond, INFINITE);
		m_mutex.Lock();
	}
	
	if (m_isValid)
		return true;
	else
	{
		m_mutex.Unlock();
		return false;
	}
}

void ConditionImpl::release(int value)
{
	m_conditionnedVar = value;
	m_mutex.Unlock();
	
	signal();
}

void ConditionImpl::setValue(int value)
{
	// Make sure the Condition's value is not modified while retained
	m_mutex.Lock();
	m_conditionnedVar = value;
	m_mutex.Unlock();
	
	signal();
}
	
int ConditionImpl::value(void) const
{
	return m_conditionnedVar;
}

void ConditionImpl::signal(void)
{
	SetEvent(m_cond);
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

} // namespace sfe
