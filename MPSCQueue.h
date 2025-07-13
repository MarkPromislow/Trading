#pragma once
#include <atomic>
#include <cmath>
#include <vector>

template <typename T>
class MPSCQueue
{
private:
	std::vector<T> m_buffer;
	unsigned m_capacity;
	unsigned m_mask;
	unsigned m_nextWriteIdx{ 0 };
	std::atomic<unsigned> m_aNextWriteIdx{ 0 };
	unsigned m_writeIdx{ 0 };
	std::atomic<unsigned> m_aWriteIdx{ 0 };
	alignas(64) std::atomic<unsigned> m_aReadIdx{ 0 };
	unsigned m_readIdx{ 0 };
public:
	MPSCQueue(unsigned capacity) : m_capacity(std::pow(2, ceil(log2(capacity)))), m_mask(m_capacity - 1)
	{
		m_buffer.resize(m_capacity);
	}

	bool write(T& t)
	{
		unsigned currentWriteIdx;
		unsigned nextWriteIdx;
		do
		{
			currentWriteIdx = m_aWriteIdx.load(std::memory_order_relaxed);
			nextWriteIdx = (currentWriteIdx + 1) & m_mask;
			if (nextWriteIdx == m_aReadIdx.load(std::memory_order_relaxed)) return false;
		} while (!m_aNextWriteIdx.compare_exchange_weak(currentWriteIdx, nextWriteIdx, std::memory_order_relaxed));
		m_buffer[currentWriteIdx] = t;
		m_aWriteIdx.store(nextWriteIdx, std::memory_order_release);
		return true;
	}

	T* front()
	{
		return m_readIdx != m_aWriteIdx.load(std::memory_order_acquire) ? &m_buffer[m_readIdx] : nullptr;
	}

	void pop_front()
	{
		m_readIdx = (m_readIdx + 1) & m_mask;
		m_aReadIdx.store(m_readIdx, std::memory_order_relaxed);
	}
};
