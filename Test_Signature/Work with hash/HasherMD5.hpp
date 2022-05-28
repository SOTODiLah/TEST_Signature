#pragma once

#include "IHasher.h"
#include "../CryptoPP/cryptlib.h"
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include "../CryptoPP/md5.h"

/*
*  - класс создания MD5 хэша 
*/
class HasherMD5 : public IHasher
{
public:
	std::string make(const std::string& data)
	{
		std::string hash;
		hash.resize(16);

		CryptoPP::Weak::MD5 hasher;
		hasher.Update((const CryptoPP::byte*)&data[0], data.size());
		hasher.Final((CryptoPP::byte*)&hash[0]);

		return std::move(hash);
	}
};