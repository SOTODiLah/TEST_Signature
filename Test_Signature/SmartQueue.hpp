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
    // ”даление конструкторов копировани€ и операторов присваивани€
    SmartQueue(const SmartQueue&) = delete;
    SmartQueue(SmartQueue&&) = delete;
    SmartQueue& operator=(const SmartQueue&) = delete;
    SmartQueue& operator=(SmartQueue&&) = delete;
public:
    /**
     * ќб€зательный конструктор с инициализацией вектора указанным размером + 1. ”величение размер производитс€ из-за цикличного устройства очереди.
     */
    explicit SmartQueue(size_t size) : container(size + 1)
    {
        assert(size);
    }
    
    /**
     * ћетод добавлени€ элемента в очередь. 
     */
    bool push(T& args)
    {
        //≈сли текущее значение Push равно Pop тогда контейнер переполнен.
        auto copyPushPos = pushPos.load(std::memory_order_relaxed);
        if (popPos.load(std::memory_order_acquire) == copyPushPos)
            return false;

        container[copyPushPos] = std::move(args);

        copyPushPos = incrementPosition(copyPushPos);
        pushPos.store(copyPushPos, std::memory_order_release);

        return true;
    }

    /**
     * ћетод получение элемента из очереди с его очисткой. 
     */
    bool pop(T& arg)
    {
        auto copyPopPos = popPos.load(std::memory_order_relaxed);

        //≈сли текущее положение Pop + 1 равно Push, тогда дотупных элементов нет.
        copyPopPos = incrementPosition(copyPopPos);
        if (pushPos.load(std::memory_order_acquire) == copyPopPos)
            return false;

        arg = std::move(container.at(copyPopPos));

        popPos.store(copyPopPos, std::memory_order_release);
        return true;
    }

private:
    /**
     * ћетод инкриментировани€ позиции вставки и удалени€. ѕозвол€ет за циклить позиции в переделах размера очереди.
     */
    size_t incrementPosition(size_t pos) 
    { 
        return (pos != container.size() - 1) ? pos + 1 : 0;
    }

private:
    //  онтейнер класса и атомарные переменные позиций вставки и удалени€.
    std::vector<T> container;
    std::atomic<size_t> popPos = { 0 };
    std::atomic<size_t> pushPos = { 1 };
};
