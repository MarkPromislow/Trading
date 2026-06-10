#pragma once

#include <functional>

template <typename T>
class Allocator
{
protected:
	struct Node
	{
		Node* _next;
	};

	struct Block
	{
		Block* _next;
		alignas(std::max(alignof(T), alignof(Node))) char _memory[];
	};

	void AllocateBlock()
	{
		constexpr size_t maxSize = std::max(sizeof(Node), sizeof(T));
		constexpr size_t alignment = std::max(alignof(Node), alignof(T));
		constexpr size_t slotSize = ((maxSize + alignment - 1) / alignment) * alignment;

		Block* block = reinterpret_cast<Block*>(::operator new(sizeof(Block) + slotSize * _slotsPerBlock));
		block->_next = _blockList;
		_blockList = block;

		for (size_t i = 0; i < _slotsPerBlock; ++i)
		{
			Node* node = reinterpret_cast<Node*>(block->_memory + i * slotSize);
			node->_next = _freeList;
			_freeList = node;
		}
	}

	size_t _slotsPerBlock;
	Node* _freeList;
	Block* _blockList;
public:
	typedef T value_type;

	Allocator(size_t slotsPerBlock = 256) : _slotsPerBlock(slotsPerBlock), _freeList(nullptr), _blockList(nullptr) {}
	Allocator(const Allocator& other) : _slotsPerBlock(other._slotsPerBlock), _freeList(nullptr), _blockList(nullptr) {}
	Allocator(Allocator&& other) noexcept : _slotsPerBlock(other._slotsPerBlock), _freeList(other._freeList), _blockList(other._blockList)
	{
		other._freeList = nullptr;
		other._blockList = nullptr;
	}
	Allocator& operator=(const Allocator& other) = delete;

	size_t getSlotsPerBlock() const { return _slotsPerBlock; }

	template <typename U>
	constexpr Allocator(const Allocator<U>& other) : _slotsPerBlock(other.getSlotsPerBlock()), _freeList(nullptr), _blockList(nullptr) {}

	T* allocate(size_t n)
	{
		if (n == 1)
		{
			if (!_freeList)
			{
				AllocateBlock();
			}
			Node* node = _freeList;
			_freeList = node->_next;
			return reinterpret_cast<T*>(node);
		}
		else
		{
			return static_cast<T*>(::operator new(n * sizeof(T)));
		}
	}

	void deallocate(T* p, size_t n)
	{
		if (n == 1)
		{
			Node* node = reinterpret_cast<Node*>(p);
			node->_next = _freeList;
			_freeList = node;
		}
		else
		{
			::operator delete(p);
		}
	}
};

template<class T, class U>
bool operator==(const Allocator<T>& t, const Allocator<U>& u) { return &t == &u; };
template<class T, class U>
bool operator!=(const Allocator<T>& t, const Allocator<U>& u) { return &t != &u; };
