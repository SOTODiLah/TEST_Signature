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
		posInFile = fb.posInFile;
		return *this;
	}

	FileBlock& operator=(FileBlock&& fb) noexcept
	{
		str = std::move(fb.str);
		posInFile = fb.posInFile;
		return *this;
	}

	/**
	 * Вычисление позиции блока хэша в выходном файле.
	 */
	void posHash(uint64_t& posHash,const size_t& sizeDigest, const size_t& sizeBlock) const
	{ 
		if (sizeBlock == 0)
			throw std::exception("Size block is null.");
		posHash = (posInFile / sizeBlock) * sizeDigest;
	}

	/**
	 * Вычисление MD5 хэша блока файла.
	 */
	void hashMD5(std::string& digest) const
	{
		if (!str)
			throw std::exception("String not init.");
		if (digest.size() != 16)
			throw std::exception("Digest size error.");
		CryptoPP::Weak::MD5 hash;
		hash.Update((const CryptoPP::byte*)&(*str)[0], str->size());
		hash.Final((CryptoPP::byte*)&digest[0]);
	}

private:
	std::shared_ptr<std::string> str;
	uint64_t posInFile = 0;
};
