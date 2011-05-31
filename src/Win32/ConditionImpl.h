
#ifndef CONDITION_IMPL_H
#define CONDITION_IMPL_H

#include <windows.h>
#include <SFML/System.hpp>	// Use SFML mutexes

class ConditionImpl {
public:
	ConditionImpl(int var);
	~ConditionImpl(void);
	bool waitAndRetain(int value);
	void release(void);
	void setValue(int value);
	void signal(void);
	void invalidate(void);
	void restore(void);
	
private:
	int m_isValid;
	int m_conditionnedVar;
	HANDLE m_cond;
	sf::Mutex m_mutex;
};

#endif
