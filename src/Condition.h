
/*
 *  Condition.h
 *  SFE (SFML Extension) project
 *
 *  Copyright (C) 2010-2011 Soltic Lucas
 *  soltic.lucas@gmail.com
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

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
	 * waitAndRetain() returned and after you're done working on protected data,
	 * or enabling the autorelease mechanism.
	 *
	 * @awaitedValue: the value that should unlock the Condition
	 *
	 * @autorelease: Autorelease (true) to automatically release the Condition
	 * protection after it has been validated, or Manualrelease (false) to
	 * manually choose when the Condition should be released. While a Condition
	 * is retained, both waitForValueAndRetain() and operator=() will block
	 * until the Condition is released or invalidated. When a Condition is
	 * automatically released, its value is not updated.
	 *
	 * @return: true if the @awaitedValue has been reached, false otherwise.
	 * waitForValueAndRetain() may return even if @awaitedValue has not been
	 * reached if the Condition has been disabled through invalidate(). An
	 * invalidated Condition always returns in an unlocked (non-retained) state.
	 */
	bool waitForValueAndRetain(int awaitedValue, bool autorelease = false);
	
	/* Releases a previously retained (protected) Condition with @value as
	 * internal value. When the condition is released (unlocked), it is assumed
	 * to have the given value. The condition is thereafter signaled.
	 * Releasing a non-protected Condition is undefined.
	 *
	 * @value: the value the Condition should have when it is released
	 */
	void release(int value);
	
	/* Performs an assignement followed by a signal() call.
	 * The internal Condition value is updated to @value and the Condition is
	 * signaled. Note that the Condition must be unlocked (non-retained)
	 * in order to be updated, otherwise it'll block until the Condition
	 * is released.
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

