#pragma once

#include <cstddef>

namespace Intrusive
{

class LinkedListObject
{
protected:
	LinkedListObject *_prev;
	LinkedListObject *_next;

	friend class LinkedList;
	template <typename BaseType, typename Type>
	friend class ObjectPool;
public:
	LinkedListObject() : _prev(this), _next(this) {}
	bool isLinked() { return _prev != this; }
	void linkAfter(LinkedListObject *o) { o->_prev = this; o->_next = _next; _next->_prev = o; _next = o; }
	void linkBefore(LinkedListObject *o) { o->_prev = _prev; o->_next = this; _prev->_next = o; _prev = o; }
	void unlink() { _prev->_next = _next; _next->_prev = _prev; _prev = _next = this; }
	LinkedListObject *next() { return _next; }
	LinkedListObject *prev() { return _prev; }
	const LinkedListObject* next() const { return _next; }
	const LinkedListObject* prev() const { return _prev; }
};

class LinkedList: public LinkedListObject
{
protected:
	LinkedListObject _list;
public:
	LinkedList() {}
	LinkedList(const LinkedList &list) {}
	bool empty() { return _list._prev == &_list; }
	void clear() { _list._prev = _list._next = &_list; }
	void push_back(LinkedListObject *o) { o->_prev = _list._prev; o->_next = &_list; _list._prev->_next = o; _list._prev = o; }
	void push_front(LinkedListObject *o) { o->_prev = &_list; o->_next = _list._next; _list._next->_prev = o; _list._next = o; }
	LinkedListObject *begin() { return _list._next; }
	LinkedListObject *rbegin() { return _list._prev; }
	LinkedListObject *end() { return &_list; }
	LinkedListObject *pop_front() { LinkedListObject *n(0); if (_list._next != &_list) { n = _list._next; n->unlink(); } return n; }
	LinkedListObject *pop_back() { LinkedListObject *p(0); if (_list._prev != &_list) { p = _list._prev; p->unlink(); } return p; }
	size_t size() { size_t s = 0; for (LinkedListObject *o = _list._next; o != &_list; o = o->_next) ++s; return s; }
	const LinkedListObject* begin() const { return _list._next; }
	const LinkedListObject* end() const { return &_list; }
	void append(LinkedList &list);
private:
	LinkedList& operator = (const LinkedList &) = delete;
};

inline
void LinkedList::append(LinkedList &list)
{
	if (!list.empty())
	{
		list._list._next->_prev = _list._prev;
		list._list._prev->_next = &_list;
		_list._prev->_next = list._list._next;
		_list._prev = list._list._prev;
		list.clear();
	}
}

template<typename T, typename L = std::less<T>>
class SortedList
{
protected:
	LinkedList _list;
	L _less;
public:
	SortedList(L &less = L()): _less(less) {}
	inline void adjust(T *obj);
	inline void insert(T *obj);
	inline bool empty() { return _list.empty(); }
	inline void clear() { _list.clear(); }
	inline T* begin() { return static_cast<T*>(_list.begin()); }
	inline T* rbegin() { return static_cast<T*>(_list.rbegin()); }
	inline T* end() { return static_cast<T*>(_list.end()); }
	inline T* pop_front() { return static_cast<T*>(_list.pop_front()); }
	inline T* pop_back() { return static_cast<T*>(_list.pop_back()); }
	size_t size() { return _list.size(); }
	bool check();
};

template<typename T, typename L>
void SortedList<T, L>::adjust(T *obj)
{
	LinkedListObject *prev = obj->prev(), *next = obj->next();
	if (prev != _list.end() && _less(*obj, *static_cast<T*>(prev)))
	{
		obj->unlink();
		for (prev = prev->prev(); prev != _list.end(); prev = prev->prev())
		{
			if (_less(*static_cast<T*>(prev), *obj)) break;
		}
		prev->link(obj);
	}
	else if(next != _list.end() && _less(*static_cast<T*>(next), *obj))
	{
		obj->unlink();
		for (next = next->next(); next != _list.end(); next = next->next())
		{
			if (_less(*obj, *static_cast<T*>(next))) break;
		}
		next->linkBefore(obj);
	}
}

template<typename T, typename L>
void SortedList<T, L>::insert(T *obj)
{
	LinkedListObject *next = _list.begin();
	for (; next != _list.end(); next = next->next())
	{
		if (_less(*obj, *static_cast<T*>(next))) break;
	}
	next->linkBefore(obj);
}

template<typename T, typename L>
bool SortedList<T, L>::check()
{
	LinkedListObject *next = _list.begin();
	if (next != _list.end())
	{
		for (next = next->next(); next != _list.end(); next = next->next())
		{
			if (_less(*static_cast<T*>(next), *static_cast<T*>(next->prev()))) return false;
		}
	}
	return true;
}

template<typename T, typename L = std::less<T>>
class MinimumSortedList : public SortedList<T, L>
{
protected:
	size_t _maxSize;
	size_t _size;
public:
	MinimumSortedList(size_t maxSize = 0, L &less = L()) : SortedList<T, L>(less), _maxSize(maxSize), _size(0) {}
	void setMaxSize(size_t maxSize) { _maxSize = maxSize; }
	inline T *insert(T *obj);
	inline void clear() { _size = 0; _list.clear(); }
	inline T* pop_front() { if (!_list.empty()) --size;  return static_cast<T*>(_list.pop_front()); }
	inline T* pop_back() { if (!_list.empty()) --size; return static_cast<T*>(_list.pop_back()); }
	size_t size() { return _size; }
};

template<typename T, typename L>
T *MinimumSortedList<T, L>::insert(T *obj)
{
	T *removed(0);
	if (_size < _maxSize)
	{
		++_size;
		_list.insert(obj);
	}
	else if (_less(*obj, _list.back()))
	{
		removed = _list.pop_back();
		_list.insert(obj);
	}
	return removed;
}

} // namespace Intrusive
