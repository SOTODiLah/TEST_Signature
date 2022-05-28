#pragma once

#include <exception>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <condition_variable>

#include "ITask.h"
#include "../Other/MultiSingleQueue.hpp"

/*
*  - создаёт потоки в конструкторе
*  - может быть выключен с завершением оставшихся задач
*  - присоедниняет потоки в деструкторе
*/
class ThreadPool
{
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

public:
    ThreadPool() : ThreadPool(std::thread::hardware_concurrency()) {}
    ThreadPool(const int32_t& newNumberOfThread)
    {
        inProcess.test_and_set(std::memory_order::relaxed);
        size_t numberOfThread = newNumberOfThread < 0 ? 1 : newNumberOfThread;
        workers.reserve(numberOfThread);
        for (size_t i = 0; i < numberOfThread; i++)
            workers.push_back(std::thread(&ThreadPool::work, this));
    }

    ~ThreadPool()
    {
        interrupted.test_and_set(std::memory_order::relaxed); 
        shutdown();
    }

    void shutdown() noexcept
    {
        inProcess.clear(std::memory_order::relaxed);
        notifyWorkers.notify_all();
        for (auto& i : workers)
        {
            if (i.joinable())
                i.join();
        }
        workers.clear();
    }

    bool wasInterruped() const noexcept
    {
        return interrupted.test(std::memory_order::relaxed);
    }

    bool isInProcess() const noexcept
    {
        return inProcess.test(std::memory_order::relaxed);
    }

    /*
    *  - назначаем любую задачу (наследник ITask)
    *  - выбрасывает исключение, если задача пустая, была выполнена или потоки были остановлены
    */
    void assignTask(std::unique_ptr<ITask> task)
    {
        if (!isInProcess() || wasInterruped())
            throw std::exception("class ThreadPool: attempt to assign a task, when process was stoped.");
        if (!task)
            throw std::exception("class ThreadPool: attempt to assign a null pointer to task");
        if (task->isDone())
            throw std::exception("class ThreadPool: attempt to assign a task, that a task was done.");
        tasks.pushForMulti(task);
        notifyWorkers.notify_one();
    }

private:
    /*
    *  - функция работы потоков, потоки ожидают уведомления о наличии задачи
    */
    void work()
    {
        try
        {
            std::unique_ptr<ITask> task;
            std::function<void(std::unique_ptr<ITask>&)> popCallback = [this, &task](std::unique_ptr<ITask>& newTask) {
                task = std::move(newTask);
            };
            while (true)
            {
                {
                    std::unique_lock<std::mutex> lock(mtxWorker);
                    notifyWorkers.wait(lock, [this]() { return !isInProcess() || !tasks.isEmpty(); });
                    if (wasInterruped())
                        return;
                    /*
                    *  - очередь, которую используют потоки небезопасна для множества потребителей
                    *  - поэтому потоки освобождаются поочерёдно и каждый считывает отдельно от другого
                    *  - функционал очереди нужен, чтобы назначение задач было синхронизированно с потоками
                    */
                    if (!tasks.popForOne(popCallback))
                        return;
                }
                task->run();
                task.reset(); //освобождаем память до блокировки
            }
        }
        catch (const std::exception& exp) { exceptionOutput(exp.what()); }
        catch (const std::string& exp) { exceptionOutput(exp.c_str()); }
        catch (const char* exp) { exceptionOutput(exp); }
        catch (...) { exceptionOutput("unknown exception"); }
    }
    
    std::atomic_flag inProcess;
    std::atomic_flag interrupted;

    std::mutex mtxWorker;
    std::condition_variable notifyWorkers;

    std::vector<std::thread> workers;
    MultiSingleQueue<std::unique_ptr<ITask>> tasks;

    inline void exceptionOutput(const char* exp) noexcept
    {
        interrupted.test_and_set(std::memory_order::relaxed);
        inProcess.clear(std::memory_order::relaxed);
        notifyWorkers.notify_all();
        std::cout << "class ThreadPool: " << exp << '\n';
    }

};