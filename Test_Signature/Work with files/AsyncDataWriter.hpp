#pragma once

#include <exception>
#include <fstream>
#include <thread>
#include <atomic>
#include <future>
#include <mutex>
#include <queue>

#include "../Other/MultiSingleQueue.hpp"

class AsyncDataWriter
{
    AsyncDataWriter(const AsyncDataWriter&) = delete;
    AsyncDataWriter(AsyncDataWriter&&) = delete;
    AsyncDataWriter& operator=(const AsyncDataWriter&) = delete;
    AsyncDataWriter& operator=(AsyncDataWriter&&) = delete;

public:

    AsyncDataWriter(const std::string& fileName)
    {
        if (fileName == "")
            throw std::exception("class AsyncDataWriter: file name is null.");
        file.open(fileName, std::ios::binary | std::ios::out | std::ios::trunc);
        if (!file.is_open())
            throw std::exception("class AsyncDataWriter: file did not open.");
        inProcess.test_and_set();
        worker = std::thread(&AsyncDataWriter::work, this);
    }

    ~AsyncDataWriter()
    {
        interrupt();
    }

    void interrupt() noexcept
    {
        interrupted.test_and_set(std::memory_order::relaxed);
        inProcess.clear(std::memory_order::relaxed);
        join();
        file.close();
    }

    void join() noexcept
    {
        inProcess.clear(std::memory_order::relaxed);
        if (worker.joinable())
            worker.join();
    }

    bool wasInterrupted() const noexcept
    {
        return interrupted.test(std::memory_order::relaxed);
    }

    bool isInProcess() const noexcept
    {
        return inProcess.test(std::memory_order::relaxed);
    }

    void pushData(std::future<std::string> data)
    {
        if (wasInterrupted() || !isInProcess())
            throw std::exception("class AsyncDataWriter: attempt to push data, when was stoped.");
        dataset.pushForMulti(data);
    }

private:

    void work()
    {
        try
        {
            std::string data;
            std::function<void(std::future<std::string>&)> popCallback = [this, &data](std::future<std::string>& future) {
                data = std::move(future.get());
                file.write(&data[0], data.size());
            };
            while (isInProcess() || !dataset.isEmpty())
            {
                if (!dataset.popForOne(popCallback))
                    continue;
                data.clear(); // освобождаем память до блокировки
                if (!file.good())
                    break;
            }
            if (wasInterrupted())
                return;
            if (file.bad())
                throw std::exception("I/O error while reading.");
            else if (file.fail())
                throw std::exception("non-integer data encountered.");
            inProcess.clear(std::memory_order::release);
            file.close();
        }
        catch (const std::exception& exp) { exceptionOutput(exp.what()); }
        catch (const std::string& exp) { exceptionOutput(exp.c_str()); }
        catch (const char* exp) { exceptionOutput(exp); }
        catch (...) { exceptionOutput("unknown exception"); }
    }

    std::atomic_flag interrupted;
    std::atomic_flag inProcess;

    std::ofstream file;
    std::thread worker;

    MultiSingleQueue<std::future<std::string>> dataset;

    inline void exceptionOutput(const char* exp) noexcept
    {
        interrupted.test_and_set(std::memory_order::relaxed);
        inProcess.clear(std::memory_order::relaxed);
        file.close();
        std::cout << "class AsyncDataWriter: " << exp << '\n';
    }
};