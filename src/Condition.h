
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
	
	/* Wait until var == value and protect the condition.
	 * You're responsible for releasing the condition with
	 * release() after waitAndRetain() returned and you're
	 * done working on protected data.
	 * @value: the value that should unlock the condition
	 */
	void waitForValueAndRetain(int value);
	
	/* Release a previously retained (protected) condition
	 * Releasing a non-protected condition is undefined.
	 */
	void release(void);
	
	/* Signal that the condition state has changed and that
	 * threads waiting on this condition should check
	 * the new @var value.
	 */
	void signal(void);
	
private:
	ConditionImpl *m_impl;
};

#endif

