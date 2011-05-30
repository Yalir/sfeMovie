
#ifndef CONDITION_H
#define CONDITION_H

class ConditionImpl;
class Condition {
public:
	/* Initialize a Condition object that will be able to
	 * wait until var == value
	 */
	Condition(int& var);
	~Condition(void);
	
	/* Make a protected change to the value observed by the
	 * Condition
	 */
	//void setValue(T& value);
	
	// @value: the value that should unlock the condition
	void waitAndRetain(int value);
	void release(void);
	void signal(void);
	
private:
	ConditionImpl *m_impl;
};

#endif

