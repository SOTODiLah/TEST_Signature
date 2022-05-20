#pragma once

#include <memory>
#include <string>

/*
*  - интерфейс Хэшер
*/
class IHasher
{
private:
	IHasher(const IHasher&) = delete;
	IHasher(IHasher&&) = delete;
	IHasher& operator=(const IHasher&) = delete;
	IHasher& operator=(IHasher&&) = delete;

protected:
	IHasher() {}

public:
	virtual ~IHasher() {}
	virtual std::string make(const std::string& data) = 0;
};