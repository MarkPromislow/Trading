#include "DataHistory.h"

int Aggregator::initialize(uint64_t beginOffset, uint64_t endOffset, DataBucket * dataBucket)
{
	_beginOffset = beginOffset;
	_endOffset = endOffset;
	_dataBucket = dataBucket;
	return _beginOffset < _endOffset ? 0: -1;
}

inline
DataBucket* BaseBucketHistory::_bucketForOffset(unsigned offset)
{
	return reinterpret_cast<DataBucket*>((reinterpret_cast<char*>(_dataBuckets) + offset * _bucketSize));
}

BaseBucketHistory::BaseBucketHistory():
	_bucketCnt(0), _bucketDuration(0), _beginTime(0), _lastTime(0), _previousBucketInt(0), _previousTime(0), _dataBuckets(0), _previousBucket(0)
{
}

void BaseBucketHistory::addAggregator(Aggregator * aggregator)
{
	if (!aggregator->beginOffset())
	{
		// order by endOffset
		Intrusive::LinkedListObject *obj = _realTimeAggregators.begin();
		for (; obj != _realTimeAggregators.end(); obj = obj->next())
		{
			if (aggregator->endOffset() < static_cast<Aggregator*>(obj)->endOffset()) break;
		}
		obj->linkBefore(aggregator);

	}
	else
	{
		// order by startOffset
		Intrusive::LinkedListObject *obj = _aggregators.begin();
		for (; obj != _aggregators.end(); obj = obj->next())
		{
			if (aggregator->beginOffset() < static_cast<Aggregator*>(obj)->beginOffset()) break;
		}
		obj->linkBefore(aggregator);
	}
}

int BaseBucketHistory::addTime(uint64_t currentTime)
{
	if (currentTime > _previousTime)
	{
		if (currentTime > _lastTime)
		{
			if (_previousBucketInt == _bucketCnt) return 0;
			currentTime = _lastTime + 1;
		}

		unsigned currentBucketInt = static_cast<unsigned>((currentTime - _beginTime) / _bucketDuration);

		// close out prior buckets
		while (currentBucketInt > _previousBucketInt)
		{
			uint64_t bucketEndTime = _beginTime + (_previousBucketInt + 1) * _bucketDuration;

			// update buckets
			if (_previousData)
			{
				int64_t duration = bucketEndTime - _previousTime;
				_previousBucket->addDuration(duration, _previousData);

				// add data to real time aggregators
				for (Intrusive::LinkedListObject *obj = _realTimeAggregators.begin(); obj != _realTimeAggregators.end(); obj = obj->next())
				{
					Aggregator *aggregator = static_cast<Aggregator*>(obj);
					aggregator->addDuration(duration, _previousData);
				}				
			}

			// subtract buckets from real time aggregators
			for (Intrusive::LinkedListObject *obj = _realTimeAggregators.begin(); obj != _realTimeAggregators.end(); obj = obj->next())
			{
				Aggregator *aggregator = static_cast<Aggregator*>(obj);
				unsigned subBucketOffset = static_cast<unsigned>((aggregator->endOffset() - 1)/ _bucketDuration);
				if (_previousBucketInt < subBucketOffset) break;
				aggregator->subBucket(*_bucketForOffset(_previousBucketInt - subBucketOffset));
			}

			// add buckets to aggregators
			for (Intrusive::LinkedListObject *obj = _aggregators.begin(); obj != _aggregators.end(); obj = obj->next())
			{
				Aggregator *aggregator = static_cast<Aggregator*>(obj);
				unsigned addBucketOffset = static_cast<unsigned>((aggregator->beginOffset() - 1)/ _bucketDuration);
				if (_previousBucketInt < addBucketOffset) break;
				aggregator->addBucket(*_bucketForOffset(_previousBucketInt - addBucketOffset));
				// subtract buckets from aggregators
				unsigned subBucketOffset = static_cast<unsigned>((aggregator->endOffset() - 1)/ _bucketDuration);
				if (_previousBucketInt< subBucketOffset) continue;
				aggregator->subBucket(*_bucketForOffset(_previousBucketInt - subBucketOffset));
			}

			_previousTime = bucketEndTime;
			if (++_previousBucketInt == _bucketCnt) return 0;
			_previousBucket = reinterpret_cast<DataBucket*>(reinterpret_cast<char*>(_previousBucket) + _bucketSize);
		}

		// update current bucket
		if (_previousData)
		{
			int64_t duration = currentTime - _previousTime;
			_previousBucket->addDuration(duration, _previousData);

			// add data to real time aggregators
			for (Intrusive::LinkedListObject *obj = _realTimeAggregators.begin(); obj != _realTimeAggregators.end(); obj = obj->next())
			{
				static_cast<Aggregator*>(obj)->addDuration(duration, _previousData);
			}
		}
		_previousTime = currentTime;
		return 0;
	}

	return currentTime < _previousTime ? -1: 0;
}

int BaseBucketHistory::addData(uint64_t currentTime, const void *data)
{
	int result(0);

	if (currentTime < _previousTime)
	{
		if (currentTime < _beginTime) _previousData = data;
		else result = -1;
	}
	else
	{
		if (currentTime > _previousTime) addTime(currentTime);
		if (_previousBucketInt < _bucketCnt)
		{
			_previousBucket->addData(data);

			// add data to real time aggregators
			for (Intrusive::LinkedListObject *obj = _realTimeAggregators.begin(); obj != _realTimeAggregators.end(); obj = obj->next())
			{
				static_cast<Aggregator*>(obj)->addData(data);
			}
		}
		_previousData = data;
	}
	return result;
}

unsigned BaseBucketHistory::bucketOffsetForTime(uint64_t timeUnit) const
{
	unsigned offset(0);
	if (timeUnit > _beginTime)
	{
		if (timeUnit > _lastTime) offset = _bucketCnt - 1;
		else offset = static_cast<unsigned>((timeUnit - _beginTime) / _bucketDuration);
	}
	return offset;
}

DataBucket& BaseBucketHistory::bucketForOffset(unsigned offset)
{
	if (offset > _bucketCnt - 1) offset = _bucketCnt - 1;
	return *reinterpret_cast<DataBucket*>(reinterpret_cast<char*>(_dataBuckets) + offset * _bucketSize);
}

void BaseBucketHistory::reset()
{
	for (unsigned bucketInt(0); bucketInt < _bucketCnt; ++bucketInt)
	{
		_bucketForOffset(bucketInt)->reset();
	}

	for (Intrusive::LinkedListObject *obj = _realTimeAggregators.begin(); obj != _realTimeAggregators.end(); obj = obj->next())
	{
		static_cast<Aggregator*>(obj)->reset();
	}

	for (Intrusive::LinkedListObject *obj = _aggregators.begin(); obj != _aggregators.end(); obj = obj->next())
	{
		static_cast<Aggregator*>(obj)->reset();
	}

	_previousTime = _beginTime;
	_previousBucketInt = 0;
	_previousBucket = _dataBuckets;
	_previousData = 0;
}

void BaseBucketHistory::stop(uint64_t currentTime)
{
	if (_previousData && currentTime > _previousTime) addTime(currentTime);
	_previousData = 0;
}

/*
** TimedDataAggregator
*/
void TimedDataAggregator::addTime(uint64_t currentTime)
{
	if (_lastUpdateTime < currentTime) _lastUpdateTime = currentTime;

	if (!_newest)
	{
		if (_lastUpdateTime < _beginOffset || _dataQueue->empty()) return;

		TimedData *data = static_cast<TimedData*>(_dataQueue->front());
		uint64_t startTime = _lastUpdateTime - _beginOffset;
		if (startTime < data->time()) return;

		_dataBucket->addData(data);
		_newest = data;
		_newestTime = data->time();
	}

	uint64_t startTime = _lastUpdateTime - _beginOffset;
	for (; _newest->next() != _dataQueue->end();)
	{
		TimedData *data = static_cast<TimedData*>(_newest->next());
		if (startTime < data->time()) break;
		if (data->time() > _newestTime)
		{
			_dataBucket->addDuration(data->time() - _newestTime, _newest);
			_newestTime = data->time();
		}
		_dataBucket->addData(data);
		_newest = data;
	}
	if (startTime > _newestTime)
	{
		_dataBucket->addDuration(startTime - _newestTime, _newest);
		_newestTime = startTime;
	}

	// substract old data
	if (!_oldest)
	{
		if(_lastUpdateTime < _endOffset) return;
		TimedData *data = static_cast<TimedData*>(_dataQueue->front());
		uint64_t endTime = _lastUpdateTime - _endOffset;
		if (endTime < data->time()) return;

		_dataBucket->subData(data);
		_oldest = data;
		_oldestTime = data->time();
	}

	uint64_t endTime = _lastUpdateTime - _endOffset;
	for (; _oldest->next() != _dataQueue->end();)
	{
		TimedData *data = static_cast<TimedData*>(_oldest->next());
		if (endTime < data->time()) break;
		if (data->time() > _oldestTime)
		{
			_dataBucket->addDuration(_oldestTime - data->time(), _oldest);
			_oldestTime = data->time();
		}
		_dataBucket->subData(data);
		_oldest = data;
	}
	if (endTime > _oldestTime)
	{
		_dataBucket->addDuration(_oldestTime - endTime, _oldest);
		_oldestTime = endTime;
	}
}

void TimedDataAggregator::reset()
{
	Aggregator::reset();
	_oldest = _newest = 0;
	_oldestTime = _newestTime = _lastUpdateTime = 0;
}

/*
** TimedDataHistory
*/
void TimedDataHistory::addAggregator(TimedDataAggregator *aggregator)
{
	aggregator->setDataQueue(&_dataQueue);
	Intrusive::LinkedListObject *obj = _aggregators.begin();
	for (; obj != _aggregators.end(); obj = obj->next())
	{
		if (aggregator->beginOffset() < static_cast<TimedDataAggregator*>(obj)->beginOffset()) break;
	}
	obj->linkBefore(aggregator);
	if (_maximumDuration < aggregator->endOffset()) _maximumDuration = aggregator->endOffset();
}

int TimedDataHistory::addTime(uint64_t currentTime)
{
	for (Intrusive::LinkedListObject *obj = _aggregators.begin(); obj != _aggregators.end(); obj = obj->next())
	{
		static_cast<TimedDataAggregator*>(obj)->addTime(currentTime);
	}

	// trim queue
	for (; !_dataQueue.empty();)
	{
		TimedData *data = static_cast<TimedData*>(_dataQueue.front());
		if (currentTime <= data->time() + _maximumDuration || data->next() == _dataQueue.end())
			break;
		_dataQueue.pop_front();
		if (_lastData) _timedObjectPool.free(_lastData);
		_lastData = data;
	}

	return 0;
}

int TimedDataHistory::addData(TimedData *data)
{
	_dataQueue.push_back(data);
	return addTime(data->time());
}

void TimedDataHistory::reset()
{
	for (Intrusive::LinkedListObject *obj = _aggregators.begin(); obj != _aggregators.end(); obj = obj->next())
	{
		static_cast<TimedDataAggregator*>(obj)->reset();
	}

	for (; !_dataQueue.empty();)
	{
		_timedObjectPool.free(_dataQueue.pop_front());
	}
	if (_lastData) _timedObjectPool.free(_lastData);
	_lastData = 0;
}

TimedDataHistory::~TimedDataHistory()
{
	reset();
}
