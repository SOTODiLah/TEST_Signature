#pragma once

#include <queue>
#include <mutex>

/*
*  Multi producers - Single consumer Queue
*/
template<typename T>
class MultiSingleQueue
{
public:
	MultiSingleQueue(){}
	~MultiSingleQueue(){}

	bool isEmpty() const noexcept
	{
		return queueData.empty();
	}

	void pushForMulti(T& data)
	{
		std::scoped_lock<std::mutex> lock(mtxQueue);
		queueData.push(std::move(data));
	}

	/*
	*  - использует функцию обратного вызова, которая позволяет обработать данные раньше,
	*  - чем придётся ждать блокировки и разблокировки данных
	*/
	bool popForOne(std::function<void(T&)>& func)
	{
		if (queueData.empty())
			return false;
		func(queueData.front());
		
		/*
		*  - поскольку потребитель только один
		*  - блокируем только операцию удаления
		*/
		std::scoped_lock<std::mutex> lock(mtxQueue);
		queueData.pop();
		return true;
	}

private:
	std::queue<T> queueData;
	std::mutex mtxQueue;
	std::mutex mtxPop;
};