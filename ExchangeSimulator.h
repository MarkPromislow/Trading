#pragma once

#include <time.h>

struct Tick {
	enum {
		Quote = 0,
		Trade = 1
	};
	int _type;
	char _symbol[12];
	timespec _ts;
};

struct Quote: public Tick
{
	unsigned _price[2];
	unsigned _size[2];
};

struct Trade : public Tick
{
	unsigned _price;
	unsigned _size;
};

#include "ExecutionReport.h"
#include "OrderBook.h"
#include "TimeManager.h"

#include <stdint.h>

struct ExecutionReportCallback
{
	virtual void onExecution(ExecutionReport &executionReport) = 0;
};

class ExchangeSimulator: public TimeEventObject
{
protected:
	struct BookOrderEqual
	{
		bool operator() (uint64_t clOrdId, const BookOrder &bookOrder) const
		{
			return clOrdId == bookOrder._orderId;
		}
	};

	struct OrderBookEqual
	{
		bool operator() (const char *symbol, const OrderBook &orderBook) const
		{
			return !strcmp(symbol, orderBook.symbol());
		}
	};

	uint64_t _delayNanoseconds;
	Intrusive::LinkedList _pendingOrderList;
	Intrusive::LinkedObjectPool<BookOrder> _bookOrderAllocator;
	Intrusive::LinkedObjectPool<PriceLevel> _priceLevelAllocator;

	Intrusive::HashTable<const char*, OrderBook, OrderBookEqual> _orderBookTable;
	Intrusive::HashTable<uint64_t, BookOrder, BookOrderEqual> _bookOrderTable;

	ExecutionReportCallback *_executionReportCallback;
	TimeManager *_timeManager;

	void status(char execType, char ordStatus, const char *text, Order *order);
public:
	ExchangeSimulator(uint64_t delayMicroseconds, size_t bookCnt, size_t orderCnt, TimeManager *timeManager);

	void setExecutionReportCallback(ExecutionReportCallback *callback) { _executionReportCallback = callback; }

	void onTick(const Tick &tick);
	void onQuote(const Quote &quote);
	void onOrder(Order *order);
	void onTrade(const Trade &trade);
	void timeEvent(const timespec &ts);
	void execute(int side, unsigned price, unsigned shares, BookOrder *bookOrder);

	PriceLevel *allocatePriceLevel() { return _priceLevelAllocator.allocate(); }
	void freePriceLevel(PriceLevel *priceLevel) { _priceLevelAllocator.free(priceLevel); }
private:
	ExchangeSimulator(const ExchangeSimulator&) = delete;
};
