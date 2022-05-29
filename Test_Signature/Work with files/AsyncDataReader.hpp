#pragma once

#include <exception>
#include <fstream>
#include <thread>
#include <atomic>
#include <functional>
#include <filesystem>


/*
*  - класс в асинхронном потоке считывает данные
*  - отдаёт данные с помощью функции обратного вызова
*/
class AsyncDataReader
{
    AsyncDataReader(const AsyncDataReader&) = delete;
    AsyncDataReader(AsyncDataReader&&) = delete;
    AsyncDataReader& operator=(const AsyncDataReader&) = delete;
    AsyncDataReader& operator=(AsyncDataReader&&) = delete;

public:

    /*
    *  - выбрасывает исключения в случае неудачного открытия файла ввода
    */
    AsyncDataReader(const std::string& fileName, const size_t& sizeBlock, const std::function<void(std::unique_ptr<std::string>&)>& func)
    {
        if (fileName == "")
            throw std::exception("class AsyncDataReader: file name is null.");
        if (!std::filesystem::exists(fileName))
            throw std::exception("class AsyncDataReader: file is not exist.");
        if (std::filesystem::is_empty(fileName))
            throw std::exception("class AsyncDataReader: file is empty.");
        file.open(fileName, std::ios::binary | std::ios::in);
        if (!file.is_open())
            throw std::exception("class AsyncDataReader: file did not open.");
        inProcess.test_and_set();
        callback = func;
        worker = std::thread(&AsyncDataReader::work, this, sizeBlock);
    }
    
    ~AsyncDataReader()
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

    void join() 
    {
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

private:

    void work(size_t sizeBlock)
    {
        try
        {
            std::unique_ptr<std::string> data;
            while (true)
            {
                data = std::make_unique<std::string>();
                data->reserve(sizeBlock);
                data->resize(sizeBlock);
                file.read(&(*data)[0], sizeBlock);
                if (!file.good())
                    break;
                callback(data);
                if (wasInterrupted())
                    return;
            }
            if (file.bad())
                throw std::exception("class AsyncDataReader: I/O error while reading.");
            else if (file.eof())
            {
                /*
                *  - последний считанный блок может быть неполным
                *  - обрезаем его до размера последних считанных байт
                */
                data->resize((size_t)file.gcount());
                callback(data);
            }
            else if (file.fail())
                throw std::exception("class AsyncDataReader: non-integer data encountered.");
            inProcess.clear(std::memory_order::relaxed);
            file.close();
        }
        catch (const std::exception& exp) { exceptionOutput(exp.what()); }
        catch (const std::string& exp) { exceptionOutput(exp.c_str()); }
        catch (const char* exp) { exceptionOutput(exp); }
        catch (...) { exceptionOutput("unknown exception"); }
    }

    std::atomic_flag interrupted;
    std::atomic_flag inProcess;

    std::ifstream file;
    std::thread worker;
    std::function<void(std::unique_ptr<std::string>&)> callback;

    inline void exceptionOutput(const char* exp) noexcept
    {
        interrupted.test_and_set(std::memory_order::relaxed);
        inProcess.clear(std::memory_order::relaxed);
        file.close();
        std::cout << "class AsyncDataReader: " << exp << '\n';
    }
};