
#include "ConditionImpl.h"

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
		WaitForSingleObject(m_cond, INFINITE);
	
	if (m_isValid)
		return true;
	else
	{
		m_mutex.Unlock();
		return false;
	}
}

void ConditionImpl::release(void)
{
	m_mutex.Unlock();
}

void ConditionImpl::setValue(int value)
{
	m_conditionnedVar = value;
	signal();
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