#pragma once

#include <stdint.h>

/*
** Thread Local mechanism for handling timed events
*/

#include "IntrusivePriorityQueue.h"

namespace GreenFrog
{

class TimeEventObject : public Intrusive::PriorityQueueObject
{
protected:
	uint64_t _time;
public:
	TimeEventObject(): _time(0) {}
	virtual void timeEvent(uint64_t time) = 0;
	uint64_t time() const {return _time;}
};

class TimeCallback : public TimeEventObject
{
protected:
	void(*_function)(void *, uint64_t);
	void *_userData;
public:
	TimeCallback(void(*function)(void*, uint64_t), void *userData): _function(function), _userData(userData) {}
	void timeEvent(uint64_t time) {_function(_userData, time); }
};

class TimeManager
{
protected:
	uint64_t _time;
	struct TimeCallbackLowerPriority
	{
		bool operator() (const TimeEventObject &lhs, TimeEventObject &rhs) const
		{
			return lhs.time() > rhs.time();
		}
	};
	Intrusive::PriorityQueue<TimeEventObject, TimeCallbackLowerPriority> _priorityQueue;
public:
	TimeManager(unsigned heapSize = 256) : _time(0), _priorityQueue(heapSize) {}
	uint64_t time() const {return _time;}

	void addCallback(TimeEventObject *cb) {_priorityQueue.push(cb);}
	void remove(TimeEventObject *cb) {_priorityQueue.erase(cb); }
	void reprioritize(TimeEventObject *cb) {_priorityQueue.reprioritize(cb);}

	void updateTime(uint64_t time)
	{
		for (TimeEventObject *cb = _priorityQueue.top(); cb; cb = _priorityQueue.top())
		{
			_time = cb->time();
			if(cb->time() > time) break;
			_priorityQueue.pop();
			cb->timeEvent(cb->time());
		}
		_time = time;
	}
};

} // namespace GreenFrog