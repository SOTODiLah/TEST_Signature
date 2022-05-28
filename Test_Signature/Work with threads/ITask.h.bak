#pragma once

#include <exception>

/*
*  - интерфейс задачи, используется в ThreadPool
*/
class ITask
{
    ITask(const ITask&) = delete;
    ITask(ITask&&) = delete;
    ITask& operator=(const ITask&) = delete;
    ITask& operator=(ITask&&) = delete;
public:
    ITask() {}
    virtual ~ITask() {}

    virtual void run() noexcept = 0;

    bool isDone() const 
    {
        return bDone;
    }
protected:
    void done()
    {
        if (bDone)
            throw std::exception("interface ITask: attempt to done task, when it was done.");
        bDone = true;
    }

private:
    bool bDone{ false };

};