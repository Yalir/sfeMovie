
/*
 *  Condition.cpp
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

#include "Condition.h"
#include <SFML/Config.hpp>

#ifdef SFML_SYSTEM_WINDOWS
#include "Win32/ConditionImpl.h"
#else
#include "Unix/ConditionImpl.h"
#endif

namespace sfe {

const bool Condition::Autorelease = true;
const bool Condition::Manualrelease = false;


Condition::Condition(int value) :
m_impl(NULL)
{
	m_impl = new ConditionImpl(value);
}

Condition::~Condition(void)
{
	delete m_impl;
}

bool Condition::waitForValueAndRetain(int awaitedValue, bool autorelease)
{
	bool flag = m_impl->waitAndRetain(awaitedValue);
	
	if (autorelease)
		m_impl->release(awaitedValue);
	
	return flag;
}

void Condition::release(int value)
{
	m_impl->release(value);
}

int Condition::operator=(int value)
{
	m_impl->setValue(value);
	return value;
}

void Condition::signal(void)
{
	m_impl->signal();
}

void Condition::invalidate(void)
{
	m_impl->invalidate();
}

void Condition::restore(void)
{
	m_impl->restore();
}
	
} // namespace sfe
