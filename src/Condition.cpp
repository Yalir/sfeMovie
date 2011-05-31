
#include "Condition.h"
#include <SFML/Config.hpp>

#ifdef SFML_SYSTEM_WINDOWS
#include "Win32/ConditionImpl.h"
#else
#include "Unix/ConditionImpl.h"
#endif

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
		m_impl->release();
	
	return flag;
}

void Condition::release(void)
{
	m_impl->release();
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
