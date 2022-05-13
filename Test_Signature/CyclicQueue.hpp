#pragma once
#include <vector>
#include <atomic>
#include <mutex>
#include <cassert>
#include <utility>
#include <functional>

template <class T>
class CyclicQueue
{
    // Удаление конструкторов копирования и операторов присваивания
    CyclicQueue(const CyclicQueue&) = delete;
    CyclicQueue(CyclicQueue&&) = delete;
    CyclicQueue& operator=(const CyclicQueue&) = delete;
    CyclicQueue& operator=(CyclicQueue&&) = delete;
public:
    /**
     * Обязательный конструктор с инициализацией вектора указанным размером + 1. Увеличение размер производится из-за цикличного устройства очереди.
     */
    explicit CyclicQueue(size_t size) : container(size + 1)
    {
        assert(size);
    }
    
    /**
     * Метод добавления элемента в очередь. 
     */
    bool push(T& args)
    {
        //Если текущее значение Push равно Pop тогда контейнер переполнен.
        auto copyPushPos = pushPos.load(std::memory_order_relaxed);
        if (popPos.load(std::memory_order_acquire) == copyPushPos)
            return false;

        container[copyPushPos] = std::move(args);

        copyPushPos = incrementPosition(copyPushPos);
        pushPos.store(copyPushPos, std::memory_order_release);

        return true;
    }

    /**
     * Метод получение элемента из очереди с его очисткой. 
     */
    bool pop(T& arg)
    {
        auto copyPopPos = popPos.load(std::memory_order_relaxed);

        //Если текущее положение Pop + 1 равно Push, тогда дотупных элементов нет.
        copyPopPos = incrementPosition(copyPopPos);
        if (pushPos.load(std::memory_order_acquire) == copyPopPos)
            return false;

        arg = std::move(container.at(copyPopPos));

        popPos.store(copyPopPos, std::memory_order_release);
        return true;
    }

private:
    /**
     * Метод инкриментирования позиции вставки и удаления. Позволяет за циклить позиции в переделах размера очереди.
     */
    size_t incrementPosition(size_t pos) 
    { 
        return (pos != container.size() - 1) ? pos + 1 : 0;
    }

private:
    // Контейнер класса и атомарные переменные позиций вставки и удаления.
    std::vector<T> container;
    std::atomic<size_t> popPos = { 0 };
    std::atomic<size_t> pushPos = { 1 };
};
