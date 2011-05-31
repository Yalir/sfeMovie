
#ifndef CONDITION_IMPL_H
#define CONDITION_IMPL_H

#include <pthread.h>

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
	pthread_cond_t m_cond;
	pthread_mutex_t m_mutex;
};

#endif
