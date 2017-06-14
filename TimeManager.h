#pragma once

/*
** Thread Local mechanism for handling timed events
*/

#include "IntrusivePriorityQueue.h"

#include <time.h>

class TimeEventObject: public Intrusive::HeapObject
{
protected:
	timespec _timespec;
public:
	TimeEventObject() : _timespec({ 0, 0 }) {}
	virtual void timeEvent(const timespec &ts) = 0;
	const timespec& time() const { return _timespec; }
};

template<typename TYPE>
void TimeCallbackFunction(void *obj, const timespec &ts)
{
	reinterpret_cast<TYPE*>(obj)->timeEvent(ts);
}

class TimeCallback: public TimeEventObject
{
protected:
	void *_obj;
	void(*_function)(void*, const timespec&);
public:
	TimeCallback(void *obj, void(*function)(void*, const timespec&)) : _obj(obj), _function(function) {}
	void timeEvent(const timespec &ts) { _function(_obj, ts); }
};

class TimeManager
{
protected:
	timespec _timespec;
	struct TimeCallbackLess
	{
		bool operator() (const TimeEventObject &l, const TimeEventObject &r) const
		{
			timespec lts(l.time()), rts(r.time());
			return (lts.tv_sec - rts.tv_sec) * 1000000000 + (lts.tv_nsec - rts.tv_nsec) < 0;
		}
	};
	Intrusive::PriorityQueue<TimeEventObject, TimeCallbackLess> _priorityQueue;
public:
	TimeManager(unsigned maxSize = 256) : _timespec({ 0, 0 }), _priorityQueue(maxSize) {}

	const timespec& time() const { return _timespec; }

	void addCallback(TimeEventObject *cb) {_priorityQueue.push(cb);}

	void reprioritize(TimeEventObject *cb) { _priorityQueue.reprioritize(cb); }

	void setTime(const timespec &ts)
	{
		for (TimeEventObject *cb = _priorityQueue.top(); cb; cb = _priorityQueue.top())
		{
			_timespec = cb->time();
			if ((_timespec.tv_sec - ts.tv_sec) * 1000000000 + (_timespec.tv_nsec - ts.tv_nsec) > 0) break;
			_priorityQueue.pop();
			cb->timeEvent(_timespec);
		}
		_timespec = ts;
	}
};
