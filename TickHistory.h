#pragma once

#include "DataHistory.h"

struct QuoteData : public TimedData
{
	unsigned _bid;
	unsigned _ask;
	unsigned _bidSize;
	unsigned _askSize;

	QuoteData() : _bid(0), _ask(0), _bidSize(0), _askSize(0) {}
	void reset() { _time = 0; _askSize = _bidSize = _ask = _bid = 0;}
};

struct TradeData : public TimedData
{
	unsigned _price;
	unsigned _size;
	QuoteData _quote;

	TradeData() : _price(0), _size(0) {}
	void reset() { _time = 0; _size = _price = 0; _quote.reset(); }
};

class QuoteDataBucket : public DataBucket
{
protected:
	uint64_t _bid;
	uint64_t _ask;
	uint64_t _bidSize;
	uint64_t _askSize;
	QuoteData _lastQuote;
public:
	QuoteDataBucket() : _bid(0), _ask(0), _bidSize(0), _askSize(0) {}

	void addData(const void *data) { _lastQuote = *static_cast<const QuoteData*>(data); ++_count; }
	void addDuration(int64_t duration, const void *data);
	DataBucket& operator += (const DataBucket &dataBucket);
	DataBucket& operator -= (const DataBucket &dataBucket);
	void reset();

	double bid() const { return _duration ? static_cast<double>(_bid) / _duration : 0; }
	double ask() const { return _duration ? static_cast<double>(_ask) / _duration : 0; }
	double bidSize() const { return _duration ? static_cast<double>(_bidSize) / _duration : 0; }
	double askSize() const { return _duration ? static_cast<double>(_askSize) / _duration : 0; }
	const QuoteData& lastQuote() const { return _lastQuote; }
};

class TradeDataBucket : public DataBucket
{
protected:
	uint64_t _value;
	uint64_t _volume;
	uint64_t _bidVolume;
	uint64_t _askVolume;
public:
	TradeDataBucket() : _value(0), _volume(0), _bidVolume(0), _askVolume(0) {}
	void addData(const void *data);
	void subData(const void *data);
	DataBucket& operator += (const DataBucket &dataBucket);
	DataBucket& operator -= (const DataBucket &dataBucket);
	void reset();

	double bidVolume() const { return _duration ? static_cast<double>(_bidVolume) / _duration : 0; }
	double askVolume() const { return _duration ? static_cast<double>(_askVolume) / _duration : 0; }
	double price() const { return _volume ? static_cast<double>(_value) / _volume : 0; }
	double volume() const { return _duration ? static_cast<double>(_volume) / _duration : 0; }
	double trades() const { return _duration ? static_cast<double>(_count) / _duration : 0; }
	double value() const { return _duration ? static_cast<double>(_value) / _duration : 0; }
};
