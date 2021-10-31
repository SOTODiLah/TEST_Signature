#pragma once
#include <iostream>
#include <time.h>

struct TimeCode
{
	uint64_t count;
	uint64_t first;
	uint64_t sum;
	bool wasFirst;
	uint64_t getFirst(bool changeOnes = false)
	{
		if (changeOnes)
		{
			if (!wasFirst)
			{
				first = clock();
				wasFirst = true;
			}
		}
		else
			first = clock();
		return first;
	}
	TimeCode()
	{
		wasFirst = true;
		getFirst(false);
		count = 0;
		sum = 0;
	}
	uint64_t operator-(uint64_t last)
	{
		return last - first;
	}
	uint64_t getLast()
	{
		wasFirst = false;
		count++;
		sum += clock() - first;
		return clock() - first;
	}
	double getTime()
	{
		return (double)sum / count;
	}
	void reset()
	{
		wasFirst = true;
		getFirst(false);
		count = 0;
		sum = 0;
	}
};