// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/** 
 * @file llevent.cpp
 * @brief LLEvent and LLEventListener base classes.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "llevent.h"

using namespace LLOldEvents;

/************************************************
    Events
************************************************/

// virtual
LLEvent::~LLEvent()
{
}

// virtual
bool LLEvent::accept(LLEventListener* listener)
{
	return true;
}

// virtual
const std::string& LLEvent::desc()
{
	return mDesc;
}

/************************************************
    Observables
************************************************/

LLObservable::LLObservable()
	: mDispatcher(new LLEventDispatcher())
{
}

// virtual
LLObservable::~LLObservable()
{
	if (mDispatcher.notNull())
	{
		mDispatcher->disengage(this);
		mDispatcher = nullptr;
	}
}

// virtual
bool LLObservable::setDispatcher(LLPointer<LLEventDispatcher> dispatcher)
{
	if (mDispatcher.notNull())
	{
		mDispatcher->disengage(this);
		mDispatcher = nullptr;
	}
	if (dispatcher.notNull() || dispatcher->engage(this))
	{
		mDispatcher = dispatcher;
		return true;
	}
	return false;
}

// Returns the current dispatcher pointer.
// virtual
LLEventDispatcher* LLObservable::getDispatcher()
{
	return mDispatcher;
}

// Notifies the dispatcher of an event being fired.
void LLObservable::fireEvent(LLPointer<LLEvent> event, LLSD filter)
{
	if (mDispatcher.notNull())
	{
		mDispatcher->fireEvent(event, filter);
	}
}

/************************************************
    Dispatchers
************************************************/

class LLEventDispatcher::Impl
{
public:
	virtual ~Impl()												{				}
	virtual bool engage(LLObservable* observable)				{ return true;	}
	virtual void disengage(LLObservable* observable)			{				}

	virtual void addListener(LLEventListener *listener, LLSD filter, const LLSD& userdata) = 0;
	virtual void removeListener(LLEventListener *listener) = 0;
	virtual std::vector<LLListenerEntry> getListeners() const = 0;
	virtual bool fireEvent(LLPointer<LLEvent> event, LLSD filter) = 0;
};

bool LLEventDispatcher::engage(LLObservable* observable)
{
	return impl->engage(observable);
}

void LLEventDispatcher::disengage(LLObservable* observable)
{
	impl->disengage(observable);
}

void LLEventDispatcher::addListener(LLEventListener *listener, LLSD filter, const LLSD& userdata)
{
	impl->addListener(listener, filter, userdata);
}

void LLEventDispatcher::removeListener(LLEventListener *listener)
{
	impl->removeListener(listener);
}

std::vector<LLListenerEntry> LLEventDispatcher::getListeners() const
{
	return impl->getListeners();
}


bool LLEventDispatcher::fireEvent(LLPointer<LLEvent> event, LLSD filter)
{
	return impl->fireEvent(event, filter);
}

class LLSimpleDispatcher : public LLEventDispatcher::Impl
{
public:
	LLSimpleDispatcher(LLEventDispatcher *parent) : mParent(parent) { }
	virtual ~LLSimpleDispatcher();
	void addListener(LLEventListener* listener, LLSD filter, const LLSD& userdata) override;
	void removeListener(LLEventListener* listener) override;
	std::vector<LLListenerEntry> getListeners() const override;
	bool fireEvent(LLPointer<LLEvent> event, LLSD filter) override;

protected:
	std::vector<LLListenerEntry> mListeners;
	LLEventDispatcher *mParent;
};

LLSimpleDispatcher::~LLSimpleDispatcher()
{
	while (mListeners.size() > 0)
	{
		removeListener(mListeners.begin()->listener);
	}
}

void LLSimpleDispatcher::addListener(LLEventListener* listener, LLSD filter, const LLSD& userdata)
{
	if (listener == nullptr) return;
	removeListener(listener);
	LLListenerEntry new_entry;
	new_entry.listener = listener;
	new_entry.filter = filter;
	new_entry.userdata = userdata;
	mListeners.push_back(new_entry);
	listener->handleAttach(mParent);
}

void LLSimpleDispatcher::removeListener(LLEventListener* listener)
{
	std::vector<LLListenerEntry>::iterator itor = mListeners.begin();
	std::vector<LLListenerEntry>::iterator end = mListeners.end();
	for (; itor != end; ++itor)
	{
		if ((*itor).listener == listener)
		{
			mListeners.erase(itor);
			break;
		}
	}
	listener->handleDetach(mParent);
}

std::vector<LLListenerEntry> LLSimpleDispatcher::getListeners() const
{
	std::vector<LLListenerEntry> ret;
	std::vector<LLListenerEntry>::const_iterator itor;
	for (itor=mListeners.begin(); itor!=mListeners.end(); ++itor)
	{
		ret.push_back(*itor);
	}
	
	return ret;
}

// virtual
bool LLSimpleDispatcher::fireEvent(LLPointer<LLEvent> event, LLSD filter)
{
	std::vector<LLListenerEntry>::iterator itor;
	std::string filter_string = filter.asString();
	for (itor=mListeners.begin(); itor!=mListeners.end(); ++itor)
	{
		LLListenerEntry& entry = *itor;
		if (filter_string.empty() || entry.filter.asString() == filter_string)
		{
			(entry.listener)->handleEvent(event, (*itor).userdata);
		}
	}
	return true;
}

LLEventDispatcher::LLEventDispatcher()
{
	impl = new LLSimpleDispatcher(this);
}

LLEventDispatcher::~LLEventDispatcher()
{
	if (impl)
	{
		delete impl;
		impl = nullptr;
	}
}

/************************************************
    Listeners
************************************************/

LLEventListener::~LLEventListener()
{
}

LLSimpleListener::~LLSimpleListener()
{
	clearDispatchers();
}

void LLSimpleListener::clearDispatchers()
{
	// Remove myself from all listening dispatchers
	std::vector<LLEventDispatcher *>::iterator itor;
	while (mDispatchers.size() > 0)
	{
		itor = mDispatchers.begin();
		LLEventDispatcher *dispatcher = *itor;
		dispatcher->removeListener(this);
		itor = mDispatchers.begin();
		if (itor != mDispatchers.end() && (*itor) == dispatcher)
		{
			// Somehow, the dispatcher was not removed. Remove it forcibly
			mDispatchers.erase(itor);
		}
	}
}

bool LLSimpleListener::handleAttach(LLEventDispatcher *dispatcher)
{
	// Add dispatcher if it doesn't already exist
	std::vector<LLEventDispatcher *>::iterator itor;
	for (itor = mDispatchers.begin(); itor != mDispatchers.end(); ++itor)
	{
		if ((*itor) == dispatcher) return true;
	}
	mDispatchers.push_back(dispatcher);
	return true;
}

bool LLSimpleListener::handleDetach(LLEventDispatcher *dispatcher)
{
	// Remove dispatcher from list
	std::vector<LLEventDispatcher *>::iterator itor;
	for (itor = mDispatchers.begin(); itor != mDispatchers.end(); )
	{
		if ((*itor) == dispatcher)
		{
			itor = mDispatchers.erase(itor);
		}
		else
		{
			++itor;
		}
	}
	return true;
}
