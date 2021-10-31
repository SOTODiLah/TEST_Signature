#pragma once
#include <vector>
#include <atomic>
#include <mutex>
#include <cassert>
#include <utility>
#include <functional>

template <class T>
class SmartQueue
{
    SmartQueue(const SmartQueue&) = delete;
    SmartQueue(SmartQueue&&) = delete;
    SmartQueue& operator=(const SmartQueue&) = delete;
    SmartQueue& operator=(SmartQueue&&) = delete;
public:
    explicit SmartQueue(size_t size) : m_container(size + 1)
    {
        assert(size);
    }

    bool push(T& args)
    {
        auto pushPos = m_pushPos.load(std::memory_order_relaxed);
        if (m_popPos.load(std::memory_order_acquire) == pushPos)
            return false;

        m_container[pushPos] = std::move(args);

        pushPos = incrementPosition(pushPos);
        m_pushPos.store(pushPos, std::memory_order_release);

        return true;
    }

    bool pop(T& arg)
    {
        auto popPos = m_popPos.load(std::memory_order_relaxed);
        popPos = incrementPosition(popPos);
        if (m_pushPos.load(std::memory_order_acquire) == popPos)
            return false;

        arg = std::move(m_container.at(popPos));

        m_popPos.store(popPos, std::memory_order_release);
        return true;
    }

private:
    size_t incrementPosition(size_t pos) 
    { 
        return (pos != m_container.size() - 1) ? pos + 1 : 0;
    }

private:
    std::vector<T> m_container;
    std::atomic<size_t> m_popPos = { 0 };
    std::atomic<size_t> m_pushPos = { 1 };
};
