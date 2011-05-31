
#include "ConditionImpl.h"
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
	
	while (m_conditionnedVar != value && m_isValid)
		pthread_cond_wait(&m_cond, &m_mutex);
	
	if (m_isValid)
		return true;
	else
	{
		pthread_mutex_unlock(&m_mutex);
		return false;
	}
}

void ConditionImpl::release(void)
{
	pthread_mutex_unlock(&m_mutex);
}

void ConditionImpl::setValue(int value)
{
	//pthread_mutex_lock(&m_mutex);
	m_conditionnedVar = value;
	signal();
	//pthread_mutex_unlock(&m_mutex);
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

