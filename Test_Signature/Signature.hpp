#pragma once

#include <exception>
#include <functional>

#include "Work with files/AsyncDataReader.hpp"
#include "Work with files/AsyncDataWriter.hpp"
#include "Work with threads/ThreadPool.hpp"
#include "Work with hash/HashTask.hpp"
#include "Work with hash/HasherMD5.hpp"
#include "Other/MultiSingleQueue.hpp"


class Signature
{
    Signature(const Signature&) = delete;
    Signature(Signature&&) = delete;
    Signature& operator=(const Signature&) = delete;
    Signature& operator=(Signature&&) = delete;

public:
    Signature(){}
    ~Signature(){}

    template<typename T = HasherMD5>
    bool generate(const std::string& inputFileName, const std::string& outputFileName, const size_t& sizeBlock) noexcept
    {
        try
        {
            MultiSingleQueue<std::string> dataOfReader;
            std::function<void(std::string&)> callbackPushOfReader = [&dataOfReader](std::string& data) {
                dataOfReader.pushForMulti(data);
            };

            AsyncDataReader reader(inputFileName, sizeBlock, callbackPushOfReader);
            AsyncDataWriter writer(outputFileName);
            ThreadPool threads(std::thread::hardware_concurrency() - 3);

            std::unique_ptr<HashTask<T>> task;
            std::function<void(std::string&)> callbackPop = [&dataOfReader, &task, &writer, &threads](std::string& data) {
                task = std::make_unique<HashTask<T>>(data);
                writer.pushData(std::move(task->getFuture()));
                threads.assignTask(std::move(task));
            };
            
            while (true)
            {
                dataOfReader.popForOne(callbackPop);
                if (!reader.isInProcess())
                    break;
                if (reader.wasInterrupted() || threads.wasInterruped() || writer.wasInterrupted())
                    return false;
            }
            while (dataOfReader.popForOne(callbackPop));

            writer.join();
            return true;
        }
        catch (const std::exception& exp) { std::cout << exp.what() << '\n'; }
        catch (const std::string& exp) { std::cout << exp.c_str() << '\n'; }
        catch (const char* exp) { std::cout << exp << '\n'; }
        catch (...) { std::cout << "unknown exception" << '\n'; }
        return false;
    }

};