#pragma once
#include <cryptoPP/cryptlib.h>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptoPP/md5.h>
#include <utility>

class FileBlock
{
public:

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

	void posHash(uint64_t& posHash,const size_t& sizeDigest, const size_t& sizeBlock) const
	{ 
		if (sizeBlock == 0)
			throw "size block is null";
		posHash = (posInFile / sizeBlock) * sizeDigest;
	}

	void hashMD5(std::string& digest) const
	{
		if (!str)
			throw "string not init";
		if (digest.size() != 16)
			throw "digest size error";
		CryptoPP::Weak::MD5 hash;
		hash.Update((const CryptoPP::byte*)&(*str)[0], str->size());
		hash.Final((CryptoPP::byte*)&digest[0]);
	}

	void set(std::shared_ptr<std::string>& newStr, const uint64_t& newPosInFile)
	{
		str = std::move(newStr);
		posInFile = newPosInFile;
	}

private:
	std::shared_ptr<std::string> str;
	uint64_t posInFile = 0;
};
