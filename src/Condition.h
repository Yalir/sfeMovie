
#ifndef CONDITION_H
#define CONDITION_H

class ConditionImpl;
class Condition {
public:
	// Constants for arg 2 of waitForValueAndRetain()
	static const bool Autorelease;		// true
	static const bool Manualrelease;	// false
	
	/* Initialize a Condition object and sets its internal value to @value.
	 * The user will thereby be able to wait on the Condition until the
	 * internal value reaches his/her awaited value
	 */
	Condition(int value = 0);
	
	/* Default destructor
	 * The Condition is invalidated before destruction
	 */
	~Condition(void);
	
	/* Waits until the Condition's value == awaitedValue and protects the Condition.
	 * You're responsible for releasing the Condition with release() after
	 * waitAndRetain() returned and you're done working on protected data.
	 *
	 * @awaitedValue: the value that should unlock the Condition
	 *
	 * @autorelease: Autorelease (true) to automatically release the Condition
	 * protection after it has been validated, or Manualrelease (false) to
	 * manually choose when the Condition should be released. When a Condition
	 * is retained, other waitForValueAndRetain() are blocked, until the
	 * Condition is released.
	 *
	 * @return: true if the @awaitedValue has been reached, false otherwise.
	 * waitForValueAndRetain() may return even if @awaitedValue has not been
	 * reached if the Condition has been disabled through invalidate(). An
	 * invalidated Condition always returns in an unlocked (non-retained) state.
	 */
	bool waitForValueAndRetain(int awaitedValue, bool autorelease = false);
	
	/* Releases a previously retained (protected) Condition.
	 * Releasing a non-protected Condition is undefined.
	 */
	void release(void);
	
	/* Performs an assignement followed by a signal() call.
	 * The internal Condition value is updated to @value and the Condition is
	 * signaled.
	 *
	 * @value: the value to be assigned to the Condition
	 *
	 * @return: @value
	 */
	int operator=(int value);
	
	/* Signal that the Condition state has changed and that
	 * threads waiting on this Condition should check
	 * the new @var value.
	 */
	void signal(void);
	
	/* Signals the Condition and disable blocking calls,
	 * thus waitForValueAndRetain() does no more wait whatever
	 * the awaitedValue is and waiting calls are unlocked, returning false.
	 */
	void invalidate(void);
	
	/* Restores the blocking capabilities of the Condition,
	 * possibly previously disabled with invalidate() 
	 */
	void restore(void);
	
private:
	ConditionImpl *m_impl;
};

#endif

