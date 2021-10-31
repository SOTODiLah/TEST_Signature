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
    // �������� ������������� ����������� � ���������� ������������
    SmartQueue(const SmartQueue&) = delete;
    SmartQueue(SmartQueue&&) = delete;
    SmartQueue& operator=(const SmartQueue&) = delete;
    SmartQueue& operator=(SmartQueue&&) = delete;
public:
    /**
     * ������������ ����������� � �������������� ������� ��������� �������� + 1. ���������� ������ ������������ ��-�� ���������� ���������� �������.
     */
    explicit SmartQueue(size_t size) : container(size + 1)
    {
        assert(size);
    }
    
    /**
     * ����� ���������� �������� � �������. 
     */
    bool push(T& args)
    {
        //���� ������� �������� Push ����� Pop ����� ��������� ����������.
        auto copyPushPos = pushPos.load(std::memory_order_relaxed);
        if (popPos.load(std::memory_order_acquire) == copyPushPos)
            return false;

        container[copyPushPos] = std::move(args);

        copyPushPos = incrementPosition(copyPushPos);
        pushPos.store(copyPushPos, std::memory_order_release);

        return true;
    }

    /**
     * ����� ��������� �������� �� ������� � ��� ��������. 
     */
    bool pop(T& arg)
    {
        auto copyPopPos = popPos.load(std::memory_order_relaxed);

        //���� ������� ��������� Pop + 1 ����� Push, ����� �������� ��������� ���.
        copyPopPos = incrementPosition(copyPopPos);
        if (pushPos.load(std::memory_order_acquire) == copyPopPos)
            return false;

        arg = std::move(container.at(copyPopPos));

        popPos.store(copyPopPos, std::memory_order_release);
        return true;
    }

private:
    /**
     * ����� ����������������� ������� ������� � ��������. ��������� �� ������� ������� � ��������� ������� �������.
     */
    size_t incrementPosition(size_t pos) 
    { 
        return (pos != container.size() - 1) ? pos + 1 : 0;
    }

private:
    // ��������� ������ � ��������� ���������� ������� ������� � ��������.
    std::vector<T> container;
    std::atomic<size_t> popPos = { 0 };
    std::atomic<size_t> pushPos = { 1 };
};
