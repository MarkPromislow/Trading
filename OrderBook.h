#pragma once

#include "IntrusiveHashTable.h"
#include "IntrusiveLinkedList.h"

#include <time.h>

/*
** OrderBook
** - works with simulated orders and market data
*/

class Order;
struct BookOrder;
class PriceLevel;
class OrderBook;
class ExchangeSimulator;

struct BookOrder : public Intrusive::LinkedListObject, public Intrusive::HashTableObject
{
	friend class PriceLevel;
	PriceLevel *_priceLevel;
	uint64_t _orderId;
	unsigned _shares;

	Order *_order;
	timespec _receivedTime;

	BookOrder(): _priceLevel(0), _orderId(0), _shares(0), _order(0) {}
	void initialize(uint64_t orderId, unsigned shares, const timespec &ts);
	void initialize(uint64_t orderId, unsigned shares, Order *order, const timespec &ts);
};

class PriceLevel : public Intrusive::LinkedListObject
{
protected:
	friend class OrderBook;
	OrderBook *_orderBook;
	unsigned _price;
	unsigned _shares;
	unsigned _orderCnt;
	Intrusive::LinkedList _orders;
public:
	PriceLevel(): _orderBook(0), _price(0), _shares(0), _orderCnt(0) {}
	void initialize(OrderBook *orderBook, unsigned price);
	unsigned price() { return _price; }
	OrderBook* orderBook() const { return _orderBook; }
	void addBookOrder(BookOrder *order) { _shares += order->_shares; ++_orderCnt; _orders.push_back(order); order->_priceLevel = this; }
	void removeBookOrder(BookOrder *order) { _shares -= order->_shares; --_orderCnt; order->unlink(); order->_priceLevel = 0; }
	bool execute(int side, BookOrder *order);
};

class OrderBook : public Intrusive::HashTableObject
{
protected:
	friend class ExchangeSimulator;
	ExchangeSimulator *_exchange;
	char _symbol[32];
	unsigned _price[2];
	unsigned _tradingMask;
	Intrusive::LinkedList _priceLevels[2];

	// identify that the top of book has changed
	bool _topOfBookFlag;

	// quote feed - orderId = 0
	BookOrder *_quoteOrders[2];
public:
	OrderBook() : _exchange(0), _tradingMask(0), _topOfBookFlag(false) { _price[0] = 0;  _price[1] = -1; _quoteOrders[0] = _quoteOrders[1] = 0; }
	void initialize(ExchangeSimulator *exchange, const char *symbol);
	ExchangeSimulator *exchange() { return _exchange; }
	const char *symbol() const { return _symbol; }
	unsigned bid() const { return _price[0]; }
	unsigned ask() const { return _price[1]; }

	bool newOrder(int side, unsigned price, BookOrder *order);
	bool replaceRequest(int side, unsigned price, BookOrder *order, BookOrder *referenceOrder);
	void cancelRequest(int side, BookOrder *referenceOrder);

	void execute(int side, unsigned price, unsigned shares, BookOrder *order);

	void display();
};

inline
void BookOrder::initialize(uint64_t orderId, unsigned shares, const timespec &ts)
{
	_priceLevel = 0;
	_orderId = orderId;
	_shares = shares;
	_order = 0;
	_receivedTime = ts;
}

inline
void BookOrder::initialize(uint64_t orderId, unsigned shares, Order *order, const timespec &ts)
{
	_priceLevel = 0;
	_orderId = orderId;
	_shares = shares;
	_order = order;
	_receivedTime = ts;
}

inline
void PriceLevel::initialize(OrderBook *orderBook, unsigned price)
{
	_orderBook = orderBook;
	_price = price;
	_shares = _orderCnt = 0;
}


