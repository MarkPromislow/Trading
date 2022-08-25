#pragma once

#include <new>

namespace Intrusive
{

template <typename BaseType, typename Type>
class ObjectPool
{
protected:
	BaseType *_next;
	size_t _blockSize;

	struct Block
	{
		Block *_next;
		size_t _blockSize;
		Type *_objects;

		Type* initialize(size_t size);

		~Block();
	};
	Block *_blocks;
public:
	ObjectPool(size_t blockSize = 256) : _next(0), _blockSize(blockSize), _blocks(0) {}

	Type *allocate();
	void free(Type *object);

	size_t blockCnt() const;
	size_t blockSize() const { return _blockSize; }
	void setBlockSize(size_t blockSize) { _blockSize = blockSize; }

	~ObjectPool();
};

template <typename BaseType, typename Type>
Type* ObjectPool<BaseType, Type>::Block::initialize(size_t size)
{
	_blockSize = size;
	Type *itr;
	itr = _objects = new (static_cast<void*>(this + 1)) Type[_blockSize];
	for (Type *end = _objects + _blockSize - 1; itr < end; ++itr)
		static_cast<BaseType*>(itr)->_next = itr + 1;
	static_cast<BaseType*>(itr)->_next = 0;
	return _objects;
}

template <typename BaseType, typename Type>
ObjectPool<BaseType, Type>::Block::~Block()
{
	for (Type *itr = _objects, *end = _objects + _blockSize; itr < end; ++itr)
		itr->~Type();
}

template <typename BaseType, typename Type>
Type* ObjectPool<BaseType, Type>::allocate()
{
	BaseType *object;
	if (! (object = _next))
	{
#if _MSC_VER
		Block *block = static_cast<Block*>(operator new (sizeof(Block) + sizeof(size_t) + _blockSize * sizeof(Type)));
#else
		Block *block = static_cast<Block*>(operator new (sizeof(Block) + _blockSize * sizeof(Type)));
#endif
		block->_next = _blocks;
		_blocks = block;

		object = block->initialize(_blockSize);
	}
	_next = object->_next;
	object->_next = object;
	return static_cast<Type*>(object);
}

template <typename BaseType, typename Type>
void ObjectPool<BaseType, Type>::free(Type *object)
{
	static_cast<BaseType*>(object)->_next = _next;
	_next = object;
}

template <typename BaseType, typename Type>
size_t ObjectPool<BaseType, Type>::blockCnt() const
{
	size_t cnt(0);
	for (Block *block = _blocks; block; block = block->_next) ++cnt;
	return cnt;
}

template <typename BaseType, typename Type>
ObjectPool<BaseType, Type>::~ObjectPool()
{
	for (; _blocks;)
	{
		Block *block = _blocks;
		_blocks = block->_next;
		delete block;
	}
}

} // namespace Intrusive