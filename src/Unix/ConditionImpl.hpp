
/*
 *  ConditionImpl.hpp (Unix)
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


#ifndef CONDITION_IMPL_HPP
#define CONDITION_IMPL_HPP

#include <pthread.h>

namespace sfe {

class ConditionImpl {
public:
	ConditionImpl(int var);
	~ConditionImpl(void);
	bool waitAndRetain(int value);
	void release(int value);
	void setValue(int value);
	int value(void) const;
	void signal(void);
	void invalidate(void);
	void restore(void);
	
private:
	int m_isValid;
	int m_conditionnedVar;
	pthread_cond_t m_cond;
	pthread_mutex_t m_mutex;
};
	
} // namespace sfe

#endif
