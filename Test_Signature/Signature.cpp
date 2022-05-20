#include "Signature.h"

Signature::Signature(std::string inputFileName, std::string outputFileName, size_t sizeBlock)
	: inputFileName(inputFileName), outputFileName(outputFileName), sizeBlock(sizeBlock)
{
	canOpenInputFile = checkInputFile();
	hardwareConcurrency = std::thread::hardware_concurrency();
	if (hardwareConcurrency < 2)
		hardwareConcurrency = 2;
	halfhardwareConcurrency = hardwareConcurrency / 2;
	checkException();
}

bool Signature::checkInputFile()
{
	std::fstream file(inputFileName, std::ios::in | std::ios::binary);
	if (file.is_open())
	{
		sizeFile = std::filesystem::file_size(inputFileName);
		file.close();
		return true;
	}
	else
	{
		sizeFile = 0;
		return false;
	}
}

void Signature::checkException()
{
	if (inputFileName == "")
		throw std::exception("Invalid file name.");
	if (outputFileName == "")
		throw std::exception("Invalid file name.");
	if (!canOpenInputFile)
		throw std::exception("The file couldn't be opened.");
	if (sizeFile == 0)
		throw std::exception("The file does not exist or the size is null.");
}

bool Signature::setFilesNames(std::string newInputFileName, std::string newOutputFileName)
{
	if (newOutputFileName != "")
	{
		outputFileName = newOutputFileName;
		outputFileStream.open(outputFileName, std::ios::binary | std::ios::out | std::ios::trunc);
	}
	inputFileName = newInputFileName;
	canOpenInputFile = checkInputFile();
	return canOpenInputFile;
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

void Signature::openOutputFile()
{
	outputFileStream.open(outputFileName, std::ios::binary | std::ios::out | std::ios::trunc);
}

void Signature::writeHashToFile(const std::int64_t& position, const std::string& hash, const size_t& size)
{
	mtxForWrite.lock();
	outputFileStream.seekp(position, std::ios::beg);
	outputFileStream.write(&hash[0], size);
	mtxForWrite.unlock();
}

void Signature::signatureFileSingleReader()
{
	checkException();

	openOutputFile();
	for (size_t i = 0; i < hardwareConcurrency - 1; i++)
		cyclicQueue.push_back(std::shared_ptr<CyclicQueue<FileBlock>>(new CyclicQueue<FileBlock>(halfhardwareConcurrency)));
	threads.push_back(std::thread(&Signature::threadReader, this));
	for (size_t i = 0; i < hardwareConcurrency-1; i++)
	{
		threads.push_back(std::thread(&Signature::threadHasher, this, i));
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

		inputFileStream.close();
		finishReader = true;
	}
	catch (std::exception exp) {
		std::cout << exp.what() << '\n';
	}
}

FileBlock::HashCallback callbackHashMD5([](const std::string& str)->std::shared_ptr<std::string>
	{
		auto hash = std::shared_ptr<std::string>(new std::string());
		hash->resize(16);

		CryptoPP::Weak::MD5 hasher;
		hasher.Update((const CryptoPP::byte*)&str[0], str.size());
		hasher.Final((CryptoPP::byte*)&(*hash)[0]);

		return hash;
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
				writeHashToFile(fb.getPosHash(), fb.getHash(), fb.getHash().size());
			}
		}
	}
	catch (std::exception exp) 
	{
		std::cout << exp.what() << '\n';
	}
}

void Signature::signatureFileAllReader()
{
	checkException();
	openOutputFile();
	for (size_t i = 0; i < hardwareConcurrency; i++)
	{
		threads.push_back(std::thread(&Signature::threadReadHashWrite, this, i));
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

		uint64_t position = sizeBlock * (uint64_t)idThread;
		uint64_t positionHash = 16 * (uint64_t)idThread;
		size_t offsetPositionHash = 16 * hardwareConcurrency;
		size_t offsetPosition = (size_t)sizeBlock * (hardwareConcurrency - 1);
		uint64_t sizeFileWithoutOneBlock = sizeFile - sizeBlock;

		std::string str;
		std::string hash;
		str.resize(sizeBlock);
		hash.resize(16);

		inputFileStream.seekg(position, std::ios::beg);
		CryptoPP::Weak::MD5 hasher;

		while (position < sizeFileWithoutOneBlock)
		{
			inputFileStream.read(&str[0], sizeBlock);
			inputFileStream.seekg(offsetPosition, std::ios::cur);
			position += (uint64_t)offsetPosition + sizeBlock;

			hasher.Update((const CryptoPP::byte*)&str[0], sizeBlock);
			hasher.Final((CryptoPP::byte*)&hash[0]);

			writeHashToFile(positionHash, hash, 16);

			positionHash += offsetPositionHash;
		}
		size_t remainSize = (size_t)(sizeFile - position);
		if (remainSize > 0 && remainSize <= sizeBlock)
		{
			inputFileStream.read(&str[0], remainSize);

			hasher.Update((const CryptoPP::byte*)&str[0], remainSize);
			hasher.Final((CryptoPP::byte*)&hash[0]);

			writeHashToFile(positionHash, hash, 16);
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