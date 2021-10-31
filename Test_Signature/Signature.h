#pragma once
// ��������� ���������� ��������� ����� STL

#include <iostream>
#include <fstream>
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <filesystem>

#include <cryptoPP/cryptlib.h>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptoPP/md5.h>

#include "SmartQueue.hpp"
#include "FileBlock.hpp"

class Signature
{
private:
	// �������� ����������� � �������� ������������
	Signature() = delete;
	Signature(const Signature&) = delete;
	Signature(Signature&&) = delete;
	Signature& operator=(const Signature&) = delete;
	Signature& operator=(Signature&&) = delete;
public:
	/**
	 * ������������ ����������� � ��������������
	 */
	explicit Signature(std::string inputFileName, std::string outputFileName, size_t sizeBlock = 1048576);

	/**
		����������
	*/
	~Signature();

	// ������ ������������ ��������� ��������� (����������� ��� ������������ ������������� ������)
	bool setFilesNames(std::string newInputFileName, std::string newOutputFileName = "");
	bool setSizeBlock(size_t newSizeBlock);

	// ������ ��������� ��������� ������� (����������� ��� ������������ ������������� ������)
	const std::string& getInputFileName() const;
	const std::string& getOutputFileName() const;
	size_t getSizeBlock() const;
	uint64_t getSizeFile() const;

	/**
	 * �������� ����� ��������� ��������� �����. ���� ����� �������� �����, ��������� ������ �����������.
	 */
	void signatureFileSingleReader();

	/**
	 * ����������������� ����� ��������� ��������� �����. ������ ����� ������, ��������, ���������� ����� �����.
	 */
	void signatureFileAllReader();

	/**
	 * ����������������� ����� ��������� ��������� �����. ������ �������� � �����, ���� ��������, ������ ����������.
	 */
	void signatureFileHalfReader();
	
private:
	
	// �������� ��� ������ � �������.
	bool isOpenInputFile = false;
	std::string inputFileName;
	std::string outputFileName;
	std::ofstream outputFileStream;
	
	// ���������� ��������� �������.
	size_t hardwareConcurrency;

	// �������� ���������� ��������� �������.
	size_t halfhardwareConcurrency;

	// ������ ����� � ����� � ������.
	size_t sizeBlock;

	// ������ ����� � ������.
	uint64_t sizeFile;

	// ������ ��� ������ ������� � ������ �� ������.
	std::mutex mtxForWrite;
	
	// ������ ����� ���� (MD5 16 ����).
	const size_t sizeDigest = 16;

	// ������ �������� �������
	std::vector<std::thread> threads;

	// �������� �� ���� ������ ��������.
	bool finishReader = false;

	// ������ ��� ������ � ��������� ������ ������� ������������.
	std::vector<std::shared_ptr<SmartQueue<FileBlock>>> smartQueue;

	// ����� ������ �������� � ����� ������� ������������.
	void threadReader();
	void threadHasher(size_t idThread);
	
	// ����� ������� ������������ (������, ��������, ����������).
	void threadReadHashWrite(size_t idThread);

	// ������ ������ �������� � ���������.
	void threadForReadHalfThreadsOnTask(std::shared_ptr<std::shared_ptr<std::string>> block, size_t idThread, std::shared_ptr<bool> finish);
	void threadForHashHalfThreadsOnTask(std::shared_ptr<std::shared_ptr<std::string>> block, size_t idThread, std::shared_ptr<bool> finish);
};

