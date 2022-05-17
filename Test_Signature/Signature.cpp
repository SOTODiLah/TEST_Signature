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
	try {
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
	catch (std::exception exp) {
		std::cout << exp.what() << '\n';
	}
}

FileBlock::HashCallback callbackHashMD5([](const std::string& str)->std::shared_ptr<std::string>
	{
		auto digest = std::shared_ptr<std::string>(new std::string());
		digest->resize(16);

		CryptoPP::Weak::MD5 hash;
		hash.Update((const CryptoPP::byte*)&str[0], str.size());
		hash.Final((CryptoPP::byte*)&(*digest)[0]);

		return digest;
	});

void Signature::threadHasher(size_t idThread)
{
	try {
		FileBlock fb(nullptr, 0);
		while (!finishReader)
		{
			while (cyclicQueue[idThread]->pop(fb))
			{
				fb.hashing(callbackHashMD5, sizeBlock);

				mtxForWrite.lock();

				outputFileStream.seekp(fb.getPosHash(), std::ios::beg);
				outputFileStream.write(&fb.getHash()[0], fb.getHash().size());

				mtxForWrite.unlock();
			}
		}
	}
	catch (std::exception exp) {
		std::cout << exp.what() << '\n';
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
	try {
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
	catch (std::exception exp) {
		std::cout << exp.what() << '\n';
	}
}

Signature::~Signature()
{
	outputFileStream.close();
}