#pragma once

#include <exception>
#include <memory>
#include <string>
#include <future>

#include "../Work with threads/ITask.h"
#include "IHasher.h"

/*
*  - класса задачи хэширования, может быть исполнена только один раз
*  - инициализируем данные для хэширования в конструкторах
*/
template<typename T>
class HashTask : public ITask
{
    static_assert(std::is_base_of<IHasher,T>::value, "class HashTask: template must have inheriter from IHasher");

private:
    HashTask(const HashTask&) = delete;
    HashTask(HashTask&&) = delete;
    HashTask& operator=(const HashTask&) = delete;
    HashTask& operator=(HashTask&&) = delete;

public:

    HashTask(const char* newData)
    {
        if (newData)
            throw std::exception("class HashTask: attempt to initialize, a pointer to data is null.");
        data = std::string(newData);
    }

    HashTask(std::string& newData) 
    {
        if (newData.empty())
            throw std::exception("class HashTask: attempt to initialize, a data is null.");
        data = std::move(newData);
    }

    /*
    *  - возвращаем future, чтобы мы могли ожидать результат хэширования
    */
    std::future<std::string> getFuture() 
    {
        return hashPromise.get_future();
    }

    /*
    *  - переопределенная функция интерфейса
    *  - хэширует данные и устанавливает значение в future
    */
    void run() noexcept override
    {
        try
        {
            if (isDone())
                throw std::exception("class HashTask: attempt to run, when it was done.");
            T hasher;
            std::string hash = hasher.make(data);
            if (hash.empty())
                throw std::exception("class HashTask: hash was maked and it is null.");
            hashPromise.set_value(std::move(hash));
            done();
        }
        catch (...)
        {
            hashPromise.set_exception(std::current_exception());
        }
    }

private:
    std::string data;
    std::promise<std::string> hashPromise;
};
