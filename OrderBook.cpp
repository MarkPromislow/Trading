#define _CRT_SECURE_NO_WARNINGS

#include "OrderBook.h"
#include "ExchangeSimulator.h"

#include <stdio.h>
#include <string.h>

class Exchange
{
public:
	void execute(int side, unsigned price, unsigned shares, BookOrder *order) {}
	PriceLevel *allocatePriceLevel() { return new PriceLevel; }
	void freePriceLevel(PriceLevel *priceLevel) { delete priceLevel; }
};

namespace
{
	bool higher(unsigned l, unsigned r) { return l > r; }
	bool lower(unsigned l, unsigned r) { return l < r; }
}

inline
void OrderBook::execute(int side, unsigned price, unsigned shares, BookOrder *order)
{
	_exchange->execute(side, price, shares, order);
}

bool PriceLevel::execute(int side, BookOrder * order)
{
	// execute if the quote crosses a simulated order
	// if the quote is crossed use newer data
	bool filled(false);
	int levelSide = (side + 1) & 1;
	for (Intrusive::LinkedListObject *obj = _orders.begin(); ! filled && obj != _orders.end(); obj = _orders.begin())
	{
		BookOrder *levelOrder = static_cast<BookOrder*>(obj);
		if (order->_order)
		{
			if (levelOrder->_orderId)
			{
				if (order->_shares < levelOrder->_shares)
				{
					filled = true;
					_shares -= order->_shares;
					_orderBook->execute(side, _price, order->_shares, order);
					_orderBook->execute(levelSide, _price, order->_shares, levelOrder);
					break;
				}
				else
				{
					filled = order->_shares == levelOrder->_shares;
					_orderBook->execute(side, _price, levelOrder->_shares, order);
				}
			}
			else
			{
				filled = true;
				_orderBook->execute(side, _price, order->_shares, order);
				break;
			}
		}
		removeBookOrder(levelOrder);
		_orderBook->execute(levelSide, _price, levelOrder->_shares, levelOrder);
	}
	return filled;
}

void OrderBook::initialize(ExchangeSimulator * exchange, const char * symbol)
{
	_exchange = exchange;
	strncpy(_symbol, symbol, sizeof(_symbol));
	_tradingMask = 0;
	_price[0] = 0;
	_price[1] = -1;
}

bool OrderBook::newOrder(int side, unsigned price, BookOrder *order)
{
	bool(*inside) (unsigned, unsigned) = side ? lower : higher;

	if (! _tradingMask)
	{
		// check for cross
		int otherSide = (side + 1) & 1;
		if (!inside(_price[otherSide], price))
		{
			bool filled(false);
			Intrusive::LinkedList &levelList = _priceLevels[otherSide];
			for (Intrusive::LinkedListObject *obj = levelList.begin(); ! filled && obj != levelList.end(); obj = levelList.begin())
			{
				PriceLevel *priceLevel = static_cast<PriceLevel*>(obj);
				if (inside(priceLevel->_price, price)) break;
				filled = priceLevel->execute(side, order);
				if (priceLevel->_orderCnt) break;
				priceLevel->unlink();
				_exchange->freePriceLevel(priceLevel);
			}
			Intrusive::LinkedListObject *obj = levelList.begin();
			if (obj != levelList.end()) _price[otherSide] = static_cast<PriceLevel*>(obj)->_price;
			else _price[otherSide] = otherSide ? -1 : 0;
			if (filled) return true;
		}
	}

	// find price level
	PriceLevel *priceLevel(0);
	Intrusive::LinkedList &levelList = _priceLevels[side];
	Intrusive::LinkedListObject *obj = levelList.begin();
	for (; obj != levelList.end(); obj = obj->next())
	{
		priceLevel = static_cast<PriceLevel*>(obj);
		if (!inside(priceLevel->_price, price))
		{
			if (priceLevel->_price == price)
			{
				priceLevel->addBookOrder(order);
				obj = 0;
			}
			break;
		}
	}
	if (obj)
	{
		priceLevel = _exchange->allocatePriceLevel();
		priceLevel->initialize(this, price);
		priceLevel->addBookOrder(order);
		obj->linkBefore(priceLevel);
	}

	if (priceLevel == levelList.begin())
	{
		_price[side] = priceLevel->_price;
	}

	return false;
}

void OrderBook::cancelRequest(int side, BookOrder *referenceOrder)
{
	PriceLevel *priceLevel = referenceOrder->_priceLevel;
	Intrusive::LinkedList &levelList = _priceLevels[side];
	bool topOfBook = priceLevel == levelList.begin();
	priceLevel->removeBookOrder(referenceOrder);
	if (! priceLevel->_orderCnt)
	{
		priceLevel->unlink();
		_exchange->freePriceLevel(priceLevel);
	}

	if (topOfBook)
	{
		Intrusive::LinkedListObject *obj = levelList.begin();
		if (obj != levelList.end()) _price[side] = static_cast<PriceLevel*>(obj)->_price;
		else _price[side] = side ? -1 : 0;
	}
}

bool OrderBook::replaceRequest(int side, unsigned price, BookOrder *order, BookOrder *referenceOrder)
{
	bool filled;
	PriceLevel *priceLevel = referenceOrder->_priceLevel;
	if (priceLevel->_price == price)
	{
		priceLevel->removeBookOrder(referenceOrder);
		priceLevel->addBookOrder(order);
		filled = false;
	}
	else
	{
		cancelRequest(side, referenceOrder);
		filled = newOrder(side, price, order);
	}
	return filled;
}

void OrderBook::display()
{
	Intrusive::LinkedList &buyLevels = _priceLevels[0];
	Intrusive::LinkedList &sellLevels = _priceLevels[1];
	printf("%s %u %u\n", _symbol, _price[0], _price[1]);
	for (Intrusive::LinkedListObject *obj = sellLevels.rbegin(); obj != sellLevels.end(); obj = obj->prev())
	{
		PriceLevel *priceLevel = static_cast<PriceLevel*>(obj);
		printf("\t%u %u %u\n", priceLevel->_price, priceLevel->_shares, priceLevel->_orderCnt);
	}
	printf("\t---\n");
	for (Intrusive::LinkedListObject *obj = buyLevels.begin(); obj != buyLevels.end(); obj = obj->next())
	{
		PriceLevel *priceLevel = static_cast<PriceLevel*>(obj);
		printf("\t%u %u %u\n", priceLevel->_price, priceLevel->_shares, priceLevel->_orderCnt);
	}
}
