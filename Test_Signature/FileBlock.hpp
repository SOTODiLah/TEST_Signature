#pragma once
#include <utility>
#include <exception>
#include "../CryptoPP/cryptlib.h"
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include "../CryptoPP/md5.h"

class FileBlock {
public:
	// Коструктор по умолчанию, используется при инициализации контейнера умной очереди.
	FileBlock()
	{

	}
	
	FileBlock(std::shared_ptr<std::string>& newStr, const uint64_t& newPosInFile)
	{
		str = std::move(newStr);
		posInFile = newPosInFile;
	}

	FileBlock(std::shared_ptr<std::string>&& newStr, const uint64_t&& newPosInFile)
	{
		str = newStr;
		posInFile = newPosInFile;
	}

	FileBlock& operator=(const FileBlock& fb)
	{
		str = fb.str;
		hash = fb.hash;
		posInFile = fb.posInFile;
		posInHashFile = fb.posInHashFile;
		return *this;
	}

	FileBlock& operator=(FileBlock&& fb) noexcept
	{
		str = std::move(fb.str);
		hash = std::move(fb.hash);
		posInFile = std::move(fb.posInFile);
		posInHashFile = std::move(fb.posInHashFile);
		return *this;
	}

	 /**
	  * Вычисление хэша блока файла.
	  * Вычисление позиции блока хэша в выходном файле.
	  */
	using HashCallback = std::function<std::shared_ptr<std::string>(const std::string&)>;
	void hashing(HashCallback& callback, const size_t& sizeBlock)
	{
		if (!str)
			throw std::exception("String not init.");

		auto&& t = callback(*str);
		if (!t)
			throw std::exception("Hash not init.");

		hash = std::move(t);

		uint64_t&& strSize = str->size();

		if (strSize == 0)
			throw std::exception("String size is 0.");

		posInHashFile = (posInFile / sizeBlock) * hash->size();
	}

	const std::string& getHash() const
	{
		if (!hash)
			throw std::exception("Hash not init.");
		return *hash;
	}

	const uint64_t& getPosHash() const
	{
		return posInHashFile;
	}

private:
	std::shared_ptr<std::string> str;
	std::shared_ptr<std::string> hash;
	uint64_t posInFile = 0;
	uint64_t posInHashFile = 0;
};
