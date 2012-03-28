
/*
 *  Condition.hpp
 *  sfeMovie project
 *
 *  Copyright (C) 2010-2012 Lucas Soltic
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

#ifndef CONDITION_HPP
#define CONDITION_HPP

namespace sfe {

class ConditionImpl;
class Condition {
public:
	// Constants for arg 2 of WaitAndLock()
	static const bool AutoUnlock;	// true
	static const bool ManualUnlock;	// false
	
	/* Initializes a Condition object and sets its internal value to @value.
	 * Thus using WaitAndLock(@value, ...) will immediately return.
	 */
	Condition(int value = 0);
	
	/* Default destructor
	 * The Condition is invalidated before destruction
	 */
	~Condition(void);
	
	/* Waits until the Condition's value == awaitedValue and protects the Condition.
	 * You're responsible for unlocking the Condition with Unlock() after
	 * WaitAndLock() returned and after you're done working on protected data,
	 * or enabling the auto unlocking mechanism.
	 *
	 * The Condition locking guarantees that the condition remains true until
	 * you unlock it and that you are the only one that acquired the Condition.
	 *
	 * @awaitedValue: the value that should unlock the Condition
	 *
	 * @autoUnlock: Condition::AutoUnlock (true) to automatically unlock the Condition
	 * protection after it has been validated, or ManualUnlock (false) to
	 * manually choose when the Condition should be unlocked. While a Condition
	 * is locked, both WaitAndLock() and operator=() will block
	 * until the Condition is unlocked or invalidated. When a Condition is
	 * *automatically* unlocked, its value is not updated.
	 *
	 * @return: true if the @awaitedValue has been reached, false otherwise.
	 * WaitAndLock() may return even if @awaitedValue has not been
	 * reached if the Condition has been disabled through Invalidate(). An
	 * invalidated Condition always returns in an unlocked state.
	 */
	bool waitAndLock(int awaitedValue, bool autoUnlock = false);
	
	/* Unlocks a previously locked Condition with @value as
	 * internal value. When the condition is unlocked, it is assumed
	 * to have the given value. The condition is thereafter signaled.
	 * Unlocking a non-locked Condition is undefined.
	 *
	 * @value: the value the Condition should have when it is unlocked
	 */
	void unlock(int value);
	
	/* Performs an assignement followed by a signal() call.
	 * The internal Condition value is updated to @value and the Condition is
	 * signaled. Note that the Condition must be unlocked in order
	 * to be updated, otherwise it'll block until the Condition
	 * is unlocked.
	 *
	 * @value: the value to be assigned to the Condition
	 *
	 * @return: @value
	 */
	int operator=(int value);
	
	/* Get the current internal Condition value.
	 * This is a non-blocking call.
	 *
	 * @return: the current internal state
	 */
	int value(void) const;
	
	/* Signals that the Condition state has changed and that
	 * threads waiting on this Condition should check
	 * the new internal value.
	 */
	void signal(void);
	
	/* Signals the Condition and disables blocking calls,
	 * thus WaitAndLock() does no more wait whatever
	 * the awaitedValue is and waiting calls are unlocked, returning false.
	 */
	void invalidate(void);
	
	/* Restores the blocking capabilities of the Condition,
	 * possibly previously disabled with Invalidate() 
	 */
	void restore(void);
	
private:
	ConditionImpl *m_impl;
};

} // namespace sfe

#endif

