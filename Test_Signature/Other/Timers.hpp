#pragma once

#include <chrono>
#include <exception>
#include <iostream>

namespace Timers
{
	using namespace std::chrono;

	template<typename T = milliseconds>
	class Timer
	{
		static_assert(_Is_duration_v<T>, "class Timer: template must have std::chrono::duration");
	public:
		Timer() {}
		~Timer() {}
		
		void begin() noexcept
		{
			time = steady_clock::now();
		}

		T endDuration()
		{
			return duration_cast<T>(steady_clock::now() - time);
		}

		uint64_t end()
		{
			return endDuration().count();
		}

	private:
		time_point<steady_clock> time;
	};

	template<typename T = milliseconds>
	class AutoTimer
	{
		AutoTimer(const AutoTimer&) = delete;
		AutoTimer(AutoTimer&&) = delete;
		AutoTimer& operator=(const AutoTimer&) = delete;
		AutoTimer& operator=(AutoTimer&&) = delete;

	public:
		AutoTimer(const std::string& newMessage = "")
			: message(newMessage)
		{
			timer.begin();
		}
		~AutoTimer()
		{
			std::cout << "Time " << message << ": " << timer.end() << '\n';
		}

	private:
		Timer<T> timer;
		std::string message;
	};

	template<typename T = milliseconds>
	class AverageOfTimer
	{
		AverageOfTimer(const AverageOfTimer&) = delete;
		AverageOfTimer(AverageOfTimer&&) = delete;
		AverageOfTimer& operator=(const AverageOfTimer&) = delete;
		AverageOfTimer& operator=(AverageOfTimer&&) = delete;
	
	public:
		AverageOfTimer() {}
		~AverageOfTimer() {}

		void begin() noexcept
		{
			timer.begin();
			wasBegin = true;
		}

		/*
		*  Получить точку отсчёта.
		*  changeOnes - позволяет получить задержку в цикле до вхождения в условие
		*  Пример:
		*  while (true):
		*  {
		*		timeCode.getFirst(true); // точка отсчёта сработает только одиин раз.
		*		if (условие контролируется другим потоком)
		*		{
		*			timeCode.getLast(); // теперь точку отсчёта можны вызвать вновь.
		*		}
		*  }
		*/
		void doOnceBegin() noexcept
		{
			if (wasBegin)
				return;
			timer.begin();
			wasBegin = false;
		}

		void end() 
		{
			if (!wasBegin)
				throw std::exception("class AverageOfTimer: was no begin.");
			wasBegin = false;
			totalTime += timer.endDuration();
			count++;
		}

		uint64_t getTotalTime() const
		{
			if (count == 0)
				throw std::exception("class AverageOfTimer: attempt to get average time, when were no time measurements.");
			return totalTime.count();
		}

		long double getAverageTime() const
		{
			return (long double)getTotalTime() / count;
		}

		void outputMessage(const std::string& message = "") const
		{
			auto total = getTotalTime();
			auto average = getAverageTime();
			std::cout << "Average time " << message << ": " << average << '\n';
			std::cout << "Total time " << message << ": " << total << '\n';
		}

		void reset() noexcept
		{
			wasBegin = false;
			totalTime = T(0);
			count = 0;
		}

	private:
		bool wasBegin{ false };
		Timer<T> timer;
		T totalTime{ 0 };
		size_t count{ 0 };
	};
}