
/*
 *  Timer.hpp
 *  sfeMovie project
 *
 *  Copyright (C) 2010-2014 Lucas Soltic
 *  lucas.soltic@orange.fr
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

#ifndef SFEMOVIE_VIDEOSTREAM_HPP
#define SFEMOVIE_VIDEOSTREAM_HPP

#include <set>
#include <SFML/System.hpp>
#include "Macros.hpp"

namespace sfe {
	class Timer {
	public:
		/** The timer's status
		 */
		enum Status {
			Playing,
			Paused,
			Stopped
		};
		
		class Observer {
		public:
			/** Default constructor
			 */
			Observer(void);
			
			/** Default destructor
			 */
			virtual ~Observer(void);
			
			/** Called when playing by @a timer if this Observer is registered for notifications
			 *
			 * @param timer the timer that generated the notification
			 * @param previousStatus the timer's status before playing
			 */
			virtual void didPlay(const Timer& timer, Status previousStatus);
			
			/** Called when pausing by @a timer if this Observer is registered for notifications
			 *
			 * @param timer the timer that generated the notification
			 * @param previousStatus the timer's status before playing
			 */
			virtual void didPause(const Timer& timer, Status previousStatus);
			
			/** Called when stopping by @a timer if this Observer is registered for notifications
			 *
			 * @param timer the timer that generated the notification
			 * @param previousStatus the timer's status before playing
			 */
			virtual void didStop(const Timer& timer, Status previousStatus);
		};
		
		/** Default constructor
		 */
		Timer(void);
		
		/** Default destructor
		 *
		 * Before destruction, the timer is stopped
		 */
		~Timer(void);
		
		/** Register an observer that should be notified when this timer is
		 * played, paused or stopped
		 *
		 * @param anObserver the observer that should receive notifications
		 */
		void addObserver(Observer& anObserver);
		
		/** Stop sending notifications to this observer
		 *
		 * @param anObserver the observer that must receive no more notification
		 */
		void removeObserver(Observer& anObserver);
		
		/** Start this timer and notify all observers
		 */
		void play(void);
		
		/** Pause this timer (but do not reset it) and notify all observers
		 */
		void pause(void);
		
		/** Stop this timer and reset it and notify all observers
		 */
		void stop(void);
		
		/** Return this timer status
		 *
		 * @return Playing, Paused or Stopped
		 */
		Status getStatus(void) const;
		
		/** Return the timer's time
		 *
		 * @return the timer's time
		 */
		sf::Time getOffset(void) const;
		
	private:
		/** Notify all observers that the timer's status changed from @a oldStatus to @a newStatus
		 *
		 * @param oldStatus the timer's status before the state change
		 * @param newStatus the timer's status after the state change
		 */
		void notifyObservers(Status oldStatus, Status newStatus);
		
		sf::Time m_pausedTime;
		Status m_status;
		sf::Clock m_timer;
		std::set<Observer*> m_observers;
	};
}

#endif