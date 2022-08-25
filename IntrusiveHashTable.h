#pragma once

#include <math.h>
#include <cstddef>
#include <vector>

namespace Intrusive
{

template <class Key, class Type>
struct DefaultHashEqual
{
	bool operator() (const Key &key, const Type &type) const { return type == key; }
};

// FNV Hash Key values
enum FNV : uint64_t
{
	FNV_64_PRIME = 0x100000001B3,    //        1099511628211
	FNV_64_INIT = 0xCBF29CE484222325 // 14695981039346656037
};

template<typename Key>
struct DefaultHashFunction
{
	uint64_t operator() (const Key& key) const
	{
		uint64_t value(FNV_64_INIT);
		for (const char *itr = reinterpret_cast<const char *>(&key), *end = reinterpret_cast<const char *>(&key + 1); itr < end; ++itr)
			value = (value ^ *itr) * FNV_64_PRIME;
		return value;
	}
};

template<>
struct DefaultHashFunction<const char*>
{
	uint64_t operator() (const char* &key) const
	{
		uint64_t value(FNV_64_INIT);
		for (char c = *key; c; c = *++key)
			value = (value ^ c) * FNV_64_PRIME;
		return value;
	}
};

template<>
struct DefaultHashFunction<uint64_t>
{
	uint64_t operator() (const uint64_t &key) const
	{
		uint64_t value(FNV_64_INIT);
		const char *itr = reinterpret_cast<const char *>(&key);
		return ((((((((((((((((FNV_64_INIT ^ *itr) * FNV_64_PRIME)
			^ *(itr + 1)) * FNV_64_PRIME)
			^ *(itr + 2)) * FNV_64_PRIME)
			^ *(itr + 3)) * FNV_64_PRIME)
			^ *(itr + 4)) * FNV_64_PRIME)
			^ *(itr + 5)) * FNV_64_PRIME)
			^ *(itr + 6)) * FNV_64_PRIME)
			^ *(itr + 7)) * FNV_64_PRIME);
	}
};

class HashTableObject
{
private:
	HashTableObject *_prev;
	HashTableObject *_next;

	template<typename Key, typename Type, typename Hash, typename Equal>
	friend class HashTable;
	template<typename BaseType, typename Type>
	friend class ObjectPool;
public:
	HashTableObject(): _prev(this), _next(this) {}
	void removeFromHash() { _prev->_next = _next; _next->_prev = _prev; _prev = _next = this; }
	bool isInTable() { return _prev != this; }
};

template<typename Key, typename Type, typename Hash = DefaultHashFunction, typename Equal = DefaultHashEqual<Key, Type>>
class HashTable
{
protected:
	size_t _size;
	size_t _buckets;
	size_t _mask;

	struct HashList: public HashTableObject
	{
		void push_front(HashTableObject *o) { o->_prev = this; o->_next = _next; _next->_prev = o; _next = o; }
		size_t size() { size_t s(0); for (HashTableObject *o(_next); o != this; o = o->_next) ++s; return s; }
	};
	HashList *_listArray;

	Hash _hash;
	Equal _equal;
public:
	HashTable(size_t size, Hash &hash = Hash(), Equal &equal = Equal()):
		_size((size_t)log2(size-1)+1), _buckets(1LL << _size), _mask(_buckets - 1), _listArray(new HashList[_buckets]), _hash(hash), _equal(equal)
	{}

	bool insert(const Key &key, Type *item)
	{
		HashList &list = _listArray[(_hash(key) & _mask)];
		for (HashTableObject *o = list._next; o != &list; o = o->_next)
		{
			if (_equal(key, *static_cast<Type*>(o))) return false;
		}
		list.push_front(item);
		return true;
	}

	Type *find(const Key &key) const
	{
		HashList &list = _listArray[(_hash(key) & _mask)];
		for (HashTableObject *o = list._next; o != &list; o = o->_next)
		{
			Type *item = static_cast<Type*>(o);
			if (_equal(key, *item)) return item;
		}
		return 0;
	}

	Type *remove(const Key &key)
	{
		HashList &list = _listArray[(_hash(key) & _mask)];
		for (HashTableObject *o = list._next; o != &list; o = o->_next)
		{
			Type *item = static_cast<Type*>(o);
			if (_equal(key, *item))
			{
				o->removeFromHash();
				return item;
			}
		}
		return 0;
	}

	size_t collisions(std::vector<size_t> &collisions);

	~HashTable()
	{
		if (_listArray) delete[] _listArray;
	}
};

template<typename Key, typename Type, typename Hash, typename Equal>
size_t HashTable<Key, Type, Hash, Equal>::collisions(std::vector<size_t> &collisions)
{
	size_t cnt(0);
	collisions.clear();
	for (HashList *listItr = _listArray, *end = _listArray + _buckets; listItr < end; ++listItr)
	{
		size_t size = listItr->size();
		cnt += size;
		if(!(size < collisions.size())) collisions.resize(size+1);
		++collisions[size];
	}
	return cnt ;
}

} // namespace Intrusive

