#pragma once

#include "TickHistory.h"

void QuoteDataBucket::addDuration(int64_t duration, const void *data)
{
	const QuoteData *quoteData = static_cast<const QuoteData*>(data);
	_duration += duration;
	_bid += duration * quoteData->_bid;
	_ask += duration * quoteData->_ask;
	_bidSize += duration * quoteData->_bidSize;
	_askSize += duration * quoteData->_askSize;
}

DataBucket& QuoteDataBucket::operator+=(const DataBucket &dataBucket)
{
	const QuoteDataBucket &quoteDataBucket = static_cast<const QuoteDataBucket&>(dataBucket);
	_duration += quoteDataBucket._duration;
	_count += quoteDataBucket._count;
	_bid += quoteDataBucket._bid;
	_ask += quoteDataBucket._ask;
	_bidSize += quoteDataBucket._bidSize;
	_askSize += quoteDataBucket._askSize;
	return *this;
}

DataBucket& QuoteDataBucket::operator-=(const DataBucket &dataBucket)
{
	const QuoteDataBucket &quoteDataBucket = static_cast<const QuoteDataBucket&>(dataBucket);
	_duration -= quoteDataBucket._duration;
	_count -= quoteDataBucket._count;
	_bid -= quoteDataBucket._bid;
	_ask -= quoteDataBucket._ask;
	_bidSize -= quoteDataBucket._bidSize;
	_askSize -= quoteDataBucket._askSize;
	return *this;
}

void QuoteDataBucket::reset()
{
	DataBucket::reset();
	_askSize = _bidSize = _ask = _bid = 0;
}

void TradeDataBucket::addData(const void *data)
{
	const TradeData *tradeData = static_cast<const TradeData*>(data);
	++_count;
	uint64_t price(tradeData->_price);
	_volume += tradeData->_size;
	_value += price * tradeData->_size;
	if (2 * price > tradeData->_quote._bid + tradeData->_quote._ask) _askVolume += tradeData->_size;
	else _bidVolume += tradeData->_size;
}

void TradeDataBucket::subData(const void *data)
{
	const TradeData *tradeData = static_cast<const TradeData*>(data);
	--_count;
	uint64_t price(tradeData->_price);
	_volume -= tradeData->_size;
	_value -= price * tradeData->_size;
	if (2 * price > tradeData->_quote._bid + tradeData->_quote._ask) _askVolume -= tradeData->_size;
	else _bidVolume -= tradeData->_size;
}

DataBucket& TradeDataBucket::operator+=(const DataBucket &dataBucket)
{
	const TradeDataBucket &tradeDataBucket = static_cast<const TradeDataBucket&>(dataBucket);
	_duration += tradeDataBucket._duration;
	_count += tradeDataBucket._count;

	_volume += tradeDataBucket._volume;
	_value += tradeDataBucket._value;
	_bidVolume += tradeDataBucket._bidVolume;
	_askVolume += tradeDataBucket._askVolume;
	return *this;
}

DataBucket& TradeDataBucket::operator-=(const DataBucket &dataBucket)
{
	const TradeDataBucket &tradeDataBucket = static_cast<const TradeDataBucket&>(dataBucket);
	_duration -= tradeDataBucket._duration;
	_count -= tradeDataBucket._count;

	_volume -= tradeDataBucket._volume;
	_value -= tradeDataBucket._value;
	_bidVolume -= tradeDataBucket._bidVolume;
	_askVolume -= tradeDataBucket._askVolume;
	return *this;
}

void TradeDataBucket::reset()
{
	DataBucket::reset();
	_askVolume = _bidVolume = _value = _volume = 0;
}
