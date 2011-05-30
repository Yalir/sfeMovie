
#include "Condition.h"
#include <SFML/Config.hpp>

#ifdef SFML_SYSTEM_WINDOWS
#include "Win32/ConditionImpl.h"
#else
#include "Unix/ConditionImpl.h"
#endif


Condition::Condition(int& var) :
m_impl(NULL)
{
	m_impl = new ConditionImpl(var);
}

Condition::~Condition(void)
{
	delete m_impl;
}

void Condition::waitForValueAndRetain(int value, bool autorelease)
{
	m_impl->waitAndRetain(value);
	
	if (autorelease)
		m_impl->release();
}

void Condition::release(void)
{
	m_impl->release();
}

void Condition::operator<<(int value)
{
	m_impl->setValue(value);
}

void Condition::signal(void)
{
	m_impl->signal();
}
