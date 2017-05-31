/*
** Exchange Simulator
** - can use either top of book or order book feeds
** - trading algorithms orders interact with the Exchange Simulator book
*/

#include "ExchangeSimulator.h"
#include "FixExecType.h"
#include "FixMsgType.h"
#include "FixOrdStatus.h"

#include "Order.h"

namespace
{
// TODO: move to a common time file
timespec addTime(const timespec &ts, uint64_t nanoseconds)
{
	timespec time(ts);
	uint64_t totalNanoseconds = time.tv_nsec + nanoseconds;
	time.tv_sec += totalNanoseconds / 1000000000;
	time.tv_nsec = totalNanoseconds % 1000000000;
	return time;
}

uint64_t timespecToNanoseconds(const timespec &ts)
{
	return (ts.tv_sec % 604800) * 1000000000 + ts.tv_nsec;
}

bool less(const timespec &l, const timespec &r)
{
	return l.tv_sec < r.tv_sec || (l.tv_sec == r.tv_sec && l.tv_nsec < r.tv_nsec);
}

char *StrCpy(char *output, const char *input, size_t outputSize)
{
	char *o = output - 1;
	const char *end = input + outputSize - 2;
	for (char c = *input; c && input < end; c = *++input)
		*++o = c;
	return output;
}
} // namespace

void ExchangeSimulator::execute(int side, unsigned price, unsigned shares, BookOrder *bookOrder)
{
	bookOrder->_shares -= shares;
	Order *order;
	if ((order = bookOrder->_order))
	{
		ExecutionReport executionReport;
		executionReport._clOrdId = order->clOrdId();
		StrCpy(executionReport._symbol, order->symbol(), sizeof(executionReport._symbol));
		executionReport._lastPx = price;
		executionReport._lastQty = shares;
		executionReport._side = order->side();
		executionReport._execType = FIX::ExecType::Trade;
		if (bookOrder->_shares)
			executionReport._ordStatus = FIX::OrdStatus::PartiallyFilled;
		else
		{
			executionReport._ordStatus = FIX::OrdStatus::Filled;
			bookOrder->removeFromHash();
		}
		executionReport._transactTime = timespecToNanoseconds(_timeManager->time());
		*executionReport._text = 0;

		if (_executionReportCallback) _executionReportCallback->onExecution(executionReport);
	}
	else
	{
		printf("%s %s %d: Warning: quote executed\n", __FILE__, __FUNCTION__, __LINE__);
	}
}

void ExchangeSimulator::status(char execType, char ordStatus, const char * text, Order * order)
{
	ExecutionReport executionReport;
	executionReport._clOrdId = order->clOrdId();
	StrCpy(executionReport._symbol, order->symbol(),sizeof(executionReport._symbol));
	executionReport._lastPx = 0;
	executionReport._lastQty = 0;
	executionReport._side = order->side();
	executionReport._execType = execType;
	executionReport._ordStatus = ordStatus;
	executionReport._transactTime = timespecToNanoseconds(_timeManager->time());
	*executionReport._text = 0;
	if(text) StrCpy(executionReport._text, text, sizeof(executionReport._text));

	if (_executionReportCallback) _executionReportCallback->onExecution(executionReport);
}

ExchangeSimulator::ExchangeSimulator(uint64_t delayMicroseconds, size_t bookCnt, size_t orderCnt, TimeManager * timeManager):
	_delayNanoseconds(delayMicroseconds * 1000),
	_orderBookTable(bookCnt, OrderBookEqual()),
	_bookOrderTable(orderCnt, BookOrderEqual()),
	_executionReportCallback(0),
	_timeManager(timeManager)
{
}

void ExchangeSimulator::onOrder(Order *order)
{
	BookOrder *bookOrder = _bookOrderAllocator.allocate();
	bookOrder->initialize(order->clOrdId(), order->orderQty(), order, addTime(order->transactTime(), _delayNanoseconds));
	if (_pendingOrderList.empty())
	{
		_timespec = bookOrder->_receivedTime;
		_timeManager->addCallback(this);
	}
	_pendingOrderList.push_back(bookOrder);
}

void ExchangeSimulator::timeEvent(const timespec &ts)
{
	for (Intrusive::LinkedListObject *obj = _pendingOrderList.begin(); obj != _pendingOrderList.end(); obj = _pendingOrderList.begin())
	{
		BookOrder *bookOrder = static_cast<BookOrder*>(obj);
		if (less(ts, bookOrder->_receivedTime))
		{
			_timespec = bookOrder->_receivedTime;
			_timeManager->addCallback(this);
			break;
		}
		bookOrder->unlink();
		Order *order = bookOrder->_order;

		const char *symbol(order->symbol());
		OrderBook *orderBook(0);

		switch (order->msgType())
		{
			case FIX::MsgType::NewOrder:
			{
				if (!(orderBook = _orderBookTable.find(symbol)))
				{
					orderBook = new OrderBook();
					orderBook->initialize(this, symbol);
					_orderBookTable.insert(symbol, orderBook);
				}

				// add order to book
				if (_bookOrderTable.insert(order->clOrdId(), bookOrder))
				{
					char text[32];
					snprintf(text, sizeof(text), "clOrdId %I64u: not unique", order->clOrdId());
					status(FIX::ExecType::Rejected, FIX::OrdStatus::Rejected, text, order);
					break;
				}

				if (!orderBook->newOrder(order->sideId(), order->price(), bookOrder))
				{
					if (!bookOrder->_shares)
						status(FIX::ExecType::New, FIX::OrdStatus::New, "", order);
				}
				break;
			}
			case FIX::MsgType::CancelRequest:
			{
				BookOrder *originalBookOrder;
				uint64_t origClOrdId = order->origClOrdId();
				if (!(originalBookOrder = _bookOrderTable.find(origClOrdId)))
				{
					char buffer[32];
					snprintf(buffer, sizeof(buffer), "origClOrdId %I64u unknown", origClOrdId);
					status(FIX::ExecType::Rejected, FIX::OrdStatus::Rejected, buffer, order);
					break;
				}
				originalBookOrder->removeFromHash();
				OrderBook *originalOrderBook = originalBookOrder->_priceLevel->orderBook();
				originalOrderBook->cancelRequest(order->sideId(), originalBookOrder);
				break;
			}
			case FIX::MsgType::ReplaceRequest:
			{
				BookOrder *originalBookOrder;
				uint64_t origClOrdId = order->origClOrdId();
				if (!(originalBookOrder = _bookOrderTable.find(origClOrdId)))
				{
					char buffer[32];
					snprintf(buffer, sizeof(buffer), "origClOrdId %I64u unknown", origClOrdId);
					status(FIX::ExecType::Rejected, FIX::OrdStatus::Rejected, buffer, order);
					break;
				}

				// add to orderbook
				if (_bookOrderTable.insert(order->clOrdId(), bookOrder))
				{
					char text[32];
					snprintf(text, sizeof(text), "clOrdId %I64u: not unique", order->clOrdId());
					status(FIX::ExecType::Rejected, FIX::OrdStatus::Rejected, text, order);
					break;
				}

				originalBookOrder->removeFromHash();
				OrderBook *originalOrderBook = originalBookOrder->_priceLevel->orderBook();
				if (!originalOrderBook->replaceRequest(order->sideId(), order->price(), bookOrder, originalBookOrder))
				{
					if (originalBookOrder->_shares == bookOrder->_shares)
						status(FIX::ExecType::Replaced, FIX::OrdStatus::Replaced, "", order);
				}
			}
			case FIX::MsgType::StatusRequest:
			{
				BookOrder *originalBookOrder;
				uint64_t origClOrdId = order->origClOrdId();
				if (!(originalBookOrder = _bookOrderTable.find(origClOrdId)))
				{
					char buffer[32];
					snprintf(buffer, sizeof(buffer), "origClOrdId %I64u unknown", origClOrdId);
					status(FIX::ExecType::Rejected, FIX::OrdStatus::Rejected, buffer, order);
					break;
				}
				if(originalBookOrder->_shares == order->orderQty()) status(FIX::ExecType::New, FIX::OrdStatus::New, "", order);
				else status(FIX::ExecType::Trade, FIX::OrdStatus::PartiallyFilled, "", order);
				break;
			}
			default:
			{
				status(FIX::ExecType::Rejected, FIX::OrdStatus::Rejected, "unhandled MsgType", order);
				break;
			}
		}
	}
}

void ExchangeSimulator::onQuote(const Quote & quote)
{
	if (quote._price[0] < quote._price[1])
	{
		BookOrder *bidOrder = _bookOrderAllocator.allocate();
		BookOrder *askOrder = _bookOrderAllocator.allocate();
		bidOrder->initialize(0, quote._size[0], quote._ts);
		askOrder->initialize(0, quote._size[1], quote._ts);
		OrderBook *orderBook;
		if (!(orderBook = _orderBookTable.find(quote._symbol)))
		{
			orderBook = new OrderBook();
			orderBook->initialize(this, quote._symbol);
			_orderBookTable.insert(orderBook->symbol(), orderBook);
			orderBook->newOrder(0, quote._price[0], bidOrder);
			orderBook->newOrder(1, quote._price[1], askOrder);
		}
		else
		{
			BookOrder *oldBid(orderBook->_quoteOrders[0]), *oldAsk(orderBook->_quoteOrders[1]);
			if (quote._price[0] < oldAsk->_priceLevel->price())
			{
				orderBook->replaceRequest(0, quote._price[0], bidOrder, oldBid);
				orderBook->replaceRequest(1, quote._price[1], askOrder, oldAsk);
			}
			else
			{
				orderBook->replaceRequest(1, quote._price[1], askOrder, oldAsk);
				orderBook->replaceRequest(0, quote._price[0], bidOrder, oldBid);
			}
			_bookOrderAllocator.free(oldBid);
			_bookOrderAllocator.free(oldAsk);
		}
		orderBook->_quoteOrders[0] = bidOrder;
		orderBook->_quoteOrders[1] = askOrder;
	}
	else
	{
		printf("%s %s %d: ERROR: %s quote crossed bid %u ask %u\n", __FILE__, __FUNCTION__, __LINE__, quote._symbol, quote._price[0], quote._price[1]);
	}
}

void ExchangeSimulator::onTrade(const Trade &trade)
{

}

void ExchangeSimulator::onTick(const Tick &tick)
{
	_timeManager->setTime(tick._ts);
	switch (tick._type)
	{
	case Tick::Quote:
	{
		onQuote(static_cast<const Quote&>(tick));
		break;
	}
	case Tick::Trade:
	{
		onTrade(static_cast<const Trade&>(tick));
		break;
	}
	default:
		printf("%s %s %d: unhandled tick type: %d\n", __FILE__, __FUNCTION__, __LINE__, tick._type);
		break;
	}
}
