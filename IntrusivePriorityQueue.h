#ifndef _PRIORITY_QUEUE_
#define _PRIORITY_QUEUE_

#include <string.h>

//#include <functional>

namespace Intrusive
{

class PriorityQueueObject
{
protected:
	size_t _position;

	template<typename Type, typename Less>
	friend class PriorityQueue;
public:
	PriorityQueueObject(): _position(0) {}
	size_t position() { return _position; }
};

template<typename Type, typename Less = std::less<T> >
class PriorityQueue
{
protected:
	PriorityQueueObject **_heap;
	size_t _maxSize;
	size_t _size;
	Less _less;
public:
	PriorityQueue(unsigned maxSize, Less &less = Less()) :
		_heap(new PriorityQueueObject*[maxSize + 1]), _maxSize(maxSize), _size(0), _less(less) {*_heap = 0;}
	// remove all items from the queue
	inline void clear();
	// remove item from the queue
	inline void erase(PriorityQueueObject *item);
	// remove item at the top of the queue
	inline Type* pop();
	// add item to the queue
	inline void push(Type *item);
	// move item to new position in the queue
	inline void reprioritize(Type *item);
	// access item at the top of the queue
	inline Type *top() const {return _size ? static_cast<Type*>(_heap[1]): 0;}
	// number of items in the queue
	inline size_t size() const {return _size;}

	~PriorityQueue() { clear(); }
};

template<typename Type, typename Less>
void PriorityQueue<Type, Less>::clear()
{
	for (PriorityQueueObject **ptr(_heap + 1), **end(_heap + _size + 1); ptr < end; ++ptr)
		(*ptr)->_position = 0;
	_size = 0;
}

template<typename Type, typename Less>
void PriorityQueue<Type, Less>::erase(PriorityQueueObject *item)
{
	if (!item || !item->_position) return;


	// replace current with last
	PriorityQueueObject *lastItem = _heap[_size];
	size_t current = item->_position;
	PriorityQueueObject **currentPtr = _heap + current;
	for (size_t next(current * 2), end(_size + 1); next < end; next = (current = next) * 2)
	{
		PriorityQueueObject **nextPtr = _heap + next;
		if (_less(*static_cast<Type*>(*nextPtr), *static_cast<Type*>(*(nextPtr + 1))))
		{
			++nextPtr;
			++next;
		}

		if (_less(*static_cast<Type*>(lastItem), *static_cast<Type*>(*nextPtr)))
		{
			// move
			*currentPtr = *nextPtr;
			(*currentPtr)->_position = current;
			currentPtr = nextPtr;
		}
		else
			break;
	}
	*currentPtr = lastItem;
	lastItem->_position = current;

	item->_position = 0;
	--_size;
}

template<typename Type, typename Less>
Type* PriorityQueue<Type, Less>::pop()
{
	if (!_size) return 0;

	PriorityQueueObject **currentPtr = _heap + 1;
	PriorityQueueObject *top = *currentPtr;

	PriorityQueueObject *lastItem = _heap[_size];

	// start at top
	size_t current(1);
	for (size_t next(current * 2); next < _size; next = (current = next) * 2)
	{
		PriorityQueueObject **nextPtr = _heap + next;
		if (_less(*static_cast<Type*>(*nextPtr), *static_cast<Type*>(*(nextPtr + 1))))
		{
			++nextPtr;
			++next;
		}

		if (_less(*static_cast<Type*>(lastItem), *static_cast<Type*>(*nextPtr)))
		{
			// move
			*currentPtr = *nextPtr;
			(*currentPtr)->_position = current;
			currentPtr = nextPtr;
		}
		else
			break;
	}
	*currentPtr = lastItem;
	lastItem->_position = current;

	--_size;
	top->_position = 0;
	return static_cast<Type*>(top);
}

template<typename Type, typename Less>
void PriorityQueue<Type, Less>::push(Type *item)
{
	// check size
	if (++_size > _maxSize)
	{
		PriorityQueueObject **newHeap = new PriorityQueueObject*[2 * _maxSize + 1];
		memcpy(newHeap, _heap, (_maxSize + 1) * sizeof(PriorityQueueObject**));
		delete[] _heap;
		_heap = newHeap;
		_maxSize = 2 * _maxSize;
	}

	// start at bottom
	PriorityQueueObject **currentItem = _heap + _size;
	PriorityQueueObject **nextItem;
	size_t current(_size);
	for (size_t next(current / 2);
		next && _less(*static_cast<Type*>(*(nextItem = _heap + next)), *item);
		next = (current = next) / 2)
	{
		// move down
		*currentItem = *nextItem;
		(*currentItem)->_position = current;
		currentItem = nextItem;
	}
	*currentItem = item;
	item->PriorityQueueObject::_position = current;
}

template<typename Type, typename Less = less<Type> >
void PriorityQueue<Type, Less>::reprioritize(Type *item)
{
	size_t current = item->PriorityQueueObject::_position;
	size_t next = current / 2;
	PriorityQueueObject **currentPtr = _heap + current, **nextPtr;
	if (next && _less(*static_cast<Type*>(*(nextPtr = _heap + next)), *item))
	{
		// move next down
		*currentPtr = *nextPtr;
		(*currentPtr)->_position = current;
		currentPtr = nextPtr;

		for (next = (current = next) / 2;
		next && _less(*static_cast<Type*>(*(nextPtr = _heap + next)), *item);
			next = (current = next) / 2)
		{
			// move next down
			*currentPtr = *nextPtr;
			(*currentPtr)->_position = current;
			currentPtr = nextPtr;
		}
	}
	else
	{
		for (size_t next(current * 2), end(_size + 1); next < end; next = (current = next) * 2)
		{
			PriorityQueueObject **nextPtr = _heap + next;
			if (_less(*static_cast<Type*>(*nextPtr), *static_cast<Type*>(*(nextPtr + 1))))
			{
				++nextPtr;
				++next;
			}

			if (_less(*static_cast<Type*>(item), *static_cast<Type*>(*nextPtr)))
			{
				// move
				*currentPtr = *nextPtr;
				(*currentPtr)->_position = current;
				currentPtr = nextPtr;
			}
			else
				break;
		}
	}
	*currentPtr = item;
	item->PriorityQueueObject::_position = current;
}

} // namespace Intrusive

#endif
