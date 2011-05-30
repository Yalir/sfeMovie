
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
	void waitForValueAndRetain(int value, bool autorelease = false);
	
	/* Release a previously retained (protected) condition
	 * Releasing a non-protected condition is undefined.
	 */
	void release(void);
	
	/* @var is assigned to @value and the condition is signaled.
	 * As @var is a reference to the original variable,
	 * this function behaves like a classic assignement plus
	 * an automatic signal() call.
	 */
	void operator<<(int value);
	
	/* Signal that the condition state has changed and that
	 * threads waiting on this condition should check
	 * the new @var value.
	 */
	void signal(void);
	
private:
	ConditionImpl *m_impl;
};

#endif

