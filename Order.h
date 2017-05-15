#pragma once

#include <stdint.h>

class TradingAlgorithm;

class Order
{
protected:
	struct ChainCommon
	{
		TradingAlgorithm *_tradingAlgorithm;
		char _symbol[12];
		char _side;
		unsigned _sideId;

		// order chain status
		bool _completed;
		unsigned _cumQty;
		unsigned _exchangeCumQty;

		unsigned _referenceCnt;
	};
	ChainCommon *_chainCommon;
	unsigned _msgType;
	uint64_t _clOrdId;
	unsigned _orderQty;
	unsigned _price;
	timespec _transactTime;

	Order *_original;
	Order *_replacement;
public:
	int newOrder(const char *symbol, char side, unsigned orderQty, unsigned price, const timespec &ts);
	const char *symbol() const { return _chainCommon->_symbol; }
	char side() const { return _chainCommon->_side; }
	int sideId() const { return _chainCommon->_sideId; }
	unsigned msgType() const { return _msgType; }
	uint64_t clOrdId() const { return _clOrdId; }
	uint64_t origClOrdId() const { return _orderQty ? _original->clOrdId() : 0; }
	unsigned orderQty() const { return _orderQty; }
	unsigned price() const { return _price; }
	const timespec& transactTime() const { return _transactTime; }
	Order *original() { return _original; }
	Order *replacement() { return _replacement; }
};

inline
int Order::newOrder(const char *symbol, char side, unsigned orderQty, unsigned price, const timespec &ts)
{
	_clOrdId = 0;
	_orderQty = orderQty;
	_price = price;

	_transactTime = ts;
	_original = _replacement = 0;

	return 0;
}
