
/*
 *  Timer.cpp
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

#include "Timer.hpp"
#include "Macros.hpp"
#include "Log.hpp"

namespace sfe
{
    Timer::Observer::Observer()
    {
    }
    
    Timer::Observer::~Observer()
    {
    }
    
    void Timer::Observer::willPlay(const Timer& timer)
    {
    }
    
    void Timer::Observer::didPlay(const Timer& timer, Status previousStatus)
    {
    }
    
    void Timer::Observer::didPause(const Timer& timer, Status previousStatus)
    {
    }
    
    void Timer::Observer::didStop(const Timer& timer, Status previousStatus)
    {
    }
    
    void Timer::Observer::willSeek(const Timer& timer, sf::Time posiiton)
    {
    }
    
    void Timer::Observer::didSeek(const Timer& timer, sf::Time oldPosition)
    {
    }
    
    Timer::Timer() :
    m_pausedTime(sf::Time::Zero),
    m_status(Stopped),
    m_timer(),
    m_observers()
    {
    }
    
    Timer::~Timer()
    {
        //        if (getStatus() != Stopped)
        //            stop();
    }
    
    void Timer::addObserver(Observer& anObserver)
    {
        CHECK(m_observers.find(&anObserver) == m_observers.end(), "Timer::addObserver() - cannot add the same observer twice");
        
        m_observers.insert(&anObserver);
    }
    
    void Timer::removeObserver(Observer& anObserver)
    {
        std::set<Observer*>::iterator it = m_observers.find(&anObserver);
        
        if (it == m_observers.end())
        {
            sfeLogWarning("Timer::removeObserver() - removing an unregistered observer. Ignored.");
        }
        else
        {
            m_observers.erase(it);
        }
    }
    
    void Timer::play()
    {
        CHECK(getStatus() != Playing, "Timer::play() - timer playing twice");
        
        notifyObservers(Playing);
        
        Status oldStatus = getStatus();
        m_status = Playing;
        m_timer.restart();
        
        notifyObservers(oldStatus, getStatus());
    }
    
    void Timer::pause()
    {
        CHECK(getStatus() != Paused, "Timer::pause() - timer paused twice");
        
        Status oldStatus = getStatus();
        m_status = Paused;
        m_pausedTime += m_timer.getElapsedTime();
        
        notifyObservers(oldStatus, getStatus());
    }
    
    void Timer::stop()
    {
        CHECK(getStatus() != Stopped, "Timer::stop() - timer stopped twice");
        
        Status oldStatus = getStatus();
        m_status = Stopped;
        m_pausedTime = sf::Time::Zero;
        
        notifyObservers(oldStatus, getStatus());
        
        seek(sf::Time::Zero);
    }
    
    void Timer::seek(sf::Time position)
    {
        Status oldStatus = getStatus();
        sf::Time oldPosition = getOffset();
        
        if (oldStatus == Playing)
            pause();
        
        notifyObservers(oldPosition, position, false);
        m_pausedTime = position;
        notifyObservers(oldPosition, position, true);
        
        if (oldStatus == Playing)
            play();
    }
    
    Status Timer::getStatus() const
    {
        return m_status;
    }
    
    sf::Time Timer::getOffset() const
    {
        if (Timer::getStatus() == Playing)
            return m_pausedTime + m_timer.getElapsedTime();
        else
            return m_pausedTime;
    }
    
    void Timer::notifyObservers(Status futureStatus)
    {
        std::set<Observer*>::iterator it;
        for (it = m_observers.begin(); it != m_observers.end(); it++)
        {
            Observer* obs = *it;
            
            switch(futureStatus)
            {
                case Playing:
                    obs->willPlay(*this);
                    break;
                    
                default:
                    CHECK(false, "Timer::notifyObservers() - unhandled case in switch");
            }
        }
    }
    
    void Timer::notifyObservers(Status oldStatus, Status newStatus)
    {
        CHECK(oldStatus != newStatus, "Timer::notifyObservers() - inconsistency: no change happened");
        
        for (Observer* obs : m_observers)
        {
            
            switch(newStatus)
            {
                case Playing:
                    obs->didPlay(*this, oldStatus);
                    break;
                    
                case Paused:
                    obs->didPause(*this, oldStatus);
                    break;
                    
                case Stopped:
                    obs->didStop(*this, oldStatus);
                    break;
                default:
                    break;
            }
        }
    }
    
    void Timer::notifyObservers(sf::Time oldPosition, sf::Time newPosition, bool alreadySeeked)
    {
        CHECK(getStatus() != Playing, "inconsistency in timer");
        
		for (Observer* obs : m_observers)
        {
            if (alreadySeeked)
                obs->didSeek(*this, oldPosition);
            else
                obs->willSeek(*this, newPosition);
        }
    }
    
}
