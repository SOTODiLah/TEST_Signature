#include "Signature.h"

Signature::Signature(std::string inputFileName, std::string outputFileName, size_t sizeBlock)
	: inputFileName(inputFileName), outputFileName(outputFileName), sizeBlock(sizeBlock)
{
	isOpenInputFile = false;

	outputFileStream.open(outputFileName, std::ios::binary | std::ios::out | std::ios::trunc);

	hardwareConcurrency = std::thread::hardware_concurrency();
	if (hardwareConcurrency < 2)
		hardwareConcurrency = 2;
	halfhardwareConcurrency = hardwareConcurrency / 2;

	std::fstream file(inputFileName, std::ios::in | std::ios::binary);
	isOpenInputFile = file.is_open();
	if (isOpenInputFile)
	{
		sizeFile = std::filesystem::file_size(inputFileName);
		file.close();
	}
	else { sizeFile = 0; }
}

bool Signature::setFilesNames(std::string newInputFileName, std::string newOutputFileName)
{
	if (newOutputFileName != "")
	{
		outputFileName = newOutputFileName;
		outputFileStream.open(outputFileName, std::ios::binary | std::ios::out | std::ios::trunc);
	}
	std::fstream file(newInputFileName, std::ios::in | std::ios::binary);
	isOpenInputFile = file.is_open();
	if (isOpenInputFile)
	{
		sizeFile = std::filesystem::file_size(newInputFileName);
		inputFileName = newInputFileName;
		file.close();
	}
	else
		sizeFile = 0;
	return isOpenInputFile;
}

bool Signature::setSizeBlock(size_t newSizeBlock)
{
	if (newSizeBlock > 0 && newSizeBlock < (size_t)-2)
	{
		sizeBlock = newSizeBlock;
		return true;
	}
	return false;
}

const std::string& Signature::getInputFileName() const 
{
	return inputFileName;
}

const std::string& Signature::getOutputFileName() const
{
	return outputFileName;
}

size_t Signature::getSizeBlock() const
{
	return sizeBlock;
}

uint64_t Signature::getSizeFile() const
{
	return sizeFile;
}

void Signature::signatureFileSingleReader()
{
	if (inputFileName == "")
		throw std::exception("Invalid file name.");
	if (!isOpenInputFile)
		throw std::exception("The file couldn't be opened.");
	if (sizeFile == 0)
		throw std::exception("The file does not exist or the size is null.");
	for (size_t i = 0; i < hardwareConcurrency - 1; i++)
		cyclicQueue.push_back(std::move(std::shared_ptr<CyclicQueue<FileBlock>>(new CyclicQueue<FileBlock>(halfhardwareConcurrency)))); 
	threads.push_back(std::move(std::thread(&Signature::threadReader, this)));
	for (size_t i = 0; i < hardwareConcurrency-1; i++)
	{
		threads.push_back(std::move(std::thread(&Signature::threadHasher, this, i)));
	}
	for (auto& t : threads)
	{
		t.join();
	}
	cyclicQueue.clear();
	threads.clear();
}

void Signature::threadReader()
{
	finishReader = false;
	std::ifstream inputFileStream(inputFileName, std::ios::binary);
	uint64_t sizeFileWithoutBlock = sizeFile - sizeBlock;
	uint64_t pos = 0;
	auto str = std::shared_ptr<std::string>(new std::string);
	str->resize(sizeBlock);
	size_t idHasher = 0;
	while (pos <= sizeFileWithoutBlock)
	{
		inputFileStream.read(&(*str)[0], sizeBlock);
		FileBlock fb(str, pos);
		cyclicQueue[idHasher]->push(fb);
		idHasher = idHasher < (hardwareConcurrency - 2) ? idHasher + 1 : 0;
		str.reset(new std::string);
		str->resize(sizeBlock);
		pos += sizeBlock;
	}
	size_t smallBlock = (size_t)sizeFile - (size_t)pos;
	if (smallBlock > 0)
	{
		str->resize(smallBlock);
		inputFileStream.read(&(*str)[0], smallBlock);
		FileBlock fb(str, pos);
		cyclicQueue[idHasher]->push(fb);
	}
	finishReader = true;
}

void Signature::threadHasher(size_t idThread)
{
	FileBlock fb(nullptr, 0);
	std::string digest;
	uint64_t posHash = 0;
	digest.resize(sizeDigest);
	while (!finishReader)
	{
		while (cyclicQueue[idThread]->pop(fb))
		{
			fb.hashMD5(digest);
			fb.posHash(posHash, sizeDigest, sizeBlock);
			mtxForWrite.lock();
			outputFileStream.seekp(posHash, std::ios::beg);
			outputFileStream.write(&digest[0], sizeDigest);
			mtxForWrite.unlock();
		}
	}
}

void Signature::signatureFileAllReader()
{
	if (inputFileName == "")
		throw std::exception("Invalid file name.");
	if (!isOpenInputFile)
		throw std::exception("The file couldn't be opened.");
	if (sizeFile == 0)
		throw std::exception("The file does not exist or the size is null.");

	for (size_t i = 0; i < hardwareConcurrency; i++)
	{
		threads.push_back(std::move(std::thread(&Signature::threadReadHashWrite, this, i)));
	}
	for (auto& t : threads)
	{
		t.join();
	}
	threads.clear();
}

void Signature::threadReadHashWrite(size_t idThread)
{
	std::ifstream inputFileStream(inputFileName, std::ios::binary);
	uint64_t pos = sizeBlock * (uint64_t)idThread;
	uint64_t posHash = 16 * (uint64_t)idThread;
	size_t addPosHash = 16 * hardwareConcurrency;
	size_t addPos = (size_t)sizeBlock * (hardwareConcurrency - 1);
	uint64_t sizeFileWithoutOneBlock = sizeFile - sizeBlock;
	std::string str;
	std::string digest;
	str.resize(sizeBlock);
	digest.resize(16);
	inputFileStream.seekg(pos, std::ios::beg);
	while (pos < sizeFileWithoutOneBlock)
	{
		inputFileStream.read(&str[0], sizeBlock);
		inputFileStream.seekg(addPos, std::ios::cur);
		pos += (uint64_t)addPos + sizeBlock;

		CryptoPP::Weak::MD5 hash;
		hash.Update((const CryptoPP::byte*)&str[0], sizeBlock);
		hash.Final((CryptoPP::byte*)&digest[0]);

		mtxForWrite.lock();
		outputFileStream.seekp(posHash, std::ios::beg);
		outputFileStream.write(&digest[0], 16);
		mtxForWrite.unlock();
		posHash += addPosHash;
	}
	size_t remainSize = (size_t)(sizeFile - pos);
	if (remainSize > 0 && remainSize <= sizeBlock)
	{
		inputFileStream.read(&str[0], remainSize);
		CryptoPP::Weak::MD5 hash;
		hash.Update((const CryptoPP::byte*)&str[0], remainSize);
		hash.Final((CryptoPP::byte*)&digest[0]);
		mtxForWrite.lock();
		outputFileStream.seekp(posHash, std::ios::beg);
		outputFileStream.write(&digest[0], 16);
		mtxForWrite.unlock();
	}
	inputFileStream.close();
}

void Signature::signatureFileHalfReader()
{
	if (inputFileName == "")
		throw std::exception("Invalid file name.");
	if (!isOpenInputFile)
		throw std::exception("The file couldn't be opened.");
	if (sizeFile == 0)
		throw std::exception("The file does not exist or the size is null.");
	uint64_t realSizeFile = sizeFile;
	sizeFile = sizeFile - sizeFile % sizeBlock;
	if (realSizeFile > sizeBlock)
	{
		for (size_t i = 0; i < halfhardwareConcurrency; i++)
		{
			auto finish = std::shared_ptr<bool>(new bool);
			auto block = std::shared_ptr<std::shared_ptr<std::string>>(new std::shared_ptr<std::string>());
			*finish = false;
			threads.push_back(std::move(std::thread(&Signature::threadForReadHalfThreadsOnTask, this, block, i, finish)));
			threads.push_back(std::move(std::thread(&Signature::threadForHashHalfThreadsOnTask, this, block, i, finish)));
		}
		for (auto& t : threads)
		{
			t.join();
		}
	}
	size_t smallBlock = realSizeFile <= sizeBlock ? (size_t)realSizeFile : (size_t)realSizeFile - (size_t)sizeFile;
	if (realSizeFile > sizeFile || realSizeFile <= sizeBlock)
	{
		auto inputFileStream = std::make_shared<std::fstream>(inputFileName, std::ios::in | std::ios::binary);
		auto str = std::shared_ptr<char>(new char[smallBlock]);
		inputFileStream->seekp(sizeFile, std::ios::beg);
		inputFileStream->read(str.get(), smallBlock);
		std::string msg(str.get(), smallBlock);
		std::string digest(16, ' ');
		CryptoPP::Weak::MD5 hash;
		hash.Update((const CryptoPP::byte*)&msg[0], msg.size());
		hash.Final((CryptoPP::byte*)&digest[0]);

		uint64_t posHash = realSizeFile <= sizeBlock ? 1 : (sizeFile / sizeBlock) * 16;
		mtxForWrite.lock();
		outputFileStream.seekp(posHash, std::ios::beg);
		outputFileStream.write(digest.c_str(), digest.size());
		mtxForWrite.unlock();
	}
	sizeFile = realSizeFile;
	threads.clear();
}

void Signature::threadForReadHalfThreadsOnTask(std::shared_ptr<std::shared_ptr<std::string>> block, size_t idThread, std::shared_ptr<bool> finish)
{
	std::ifstream inputFileStream(inputFileName, std::ios::in | std::ios::binary);
	uint64_t pos = (uint64_t)idThread * sizeBlock;
	std::shared_ptr<std::string> str;
	bool doOnce = false;
	inputFileStream.seekg(pos, std::ios::beg);
	while (pos < sizeFile)
	{
		if (!doOnce)
		{
			str.reset(new std::string());
			str->resize(sizeBlock);
			doOnce = true;
			inputFileStream.read(&(*str)[0], sizeBlock);
		}
		if (!block->get())
		{
			doOnce = false;
			(*block) = std::move(str);
			pos += (uint64_t)sizeBlock * halfhardwareConcurrency;
			inputFileStream.seekg((uint64_t)sizeBlock * (halfhardwareConcurrency-1), std::ios::cur);
		}
	}
	*finish = true;
	inputFileStream.close();
}

void Signature::threadForHashHalfThreadsOnTask(std::shared_ptr<std::shared_ptr<std::string>> block, size_t idThread, std::shared_ptr<bool> finish)
{
	uint64_t posHash = (uint64_t)idThread * sizeDigest;
	std::string digest;
	digest.resize(sizeDigest);
	std::shared_ptr<std::string> msg;
	while (!(*finish) || block->get())
	{
		if (block->get())
		{
			msg = std::move(*block);

			CryptoPP::Weak::MD5 hash;
			hash.Update((const CryptoPP::byte*)&(*msg)[0], msg->size());
			hash.Final((CryptoPP::byte*)&digest[0]);
			
			mtxForWrite.lock();
			outputFileStream.seekp(posHash, std::ios::beg);
			outputFileStream.write(digest.c_str(), sizeDigest);
			mtxForWrite.unlock();
			posHash += (uint64_t)sizeDigest * (hardwareConcurrency/2);
		}
		
	}

}

Signature::~Signature()
{
	outputFileStream.close();
}