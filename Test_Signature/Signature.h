#pragma once

#include <iostream>
#include <fstream>
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <utility>

#include "../CryptoPP/cryptlib.h"
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include "../CryptoPP/md5.h"

#include "CyclicQueue.hpp"
#include "FileBlock.hpp"

class Signature
{
private:
	// Удаление конструктор и операций присваивания
	Signature() = delete;
	Signature(const Signature&) = delete;
	Signature(Signature&&) = delete;
	Signature& operator=(const Signature&) = delete;
	Signature& operator=(Signature&&) = delete;
public:
	/**
	 * Обязательный конструктор с инициализацией
	 */
	explicit Signature(std::string inputFileName, std::string outputFileName, size_t sizeBlock = 1048576);

	/**
		Деструктор
	*/
	~Signature();

	// Методы присваивания приватным свойствам (реализованы для демонстрации переносимости класса)
	bool setFilesNames(std::string newInputFileName, std::string newOutputFileName = "");
	bool setSizeBlock(size_t newSizeBlock);

	// Методы получения приватных свойств (реализованы для демонстрации переносимости класса)
	const std::string& getInputFileName() const;
	const std::string& getOutputFileName() const;
	size_t getSizeBlock() const;
	uint64_t getSizeFile() const;

	/**
	 * Основной метод получения сигнатуры файла. Один поток читатель файла, остальные потоки обработчики.
	 */
	void signatureFileSingleReader();

	/**
	 * Экспериментальный метод получения сигнатуры файла. Каждый поток читает, хэширует, записывает блоки файла.
	 */
	void signatureFileAllReader();
	
private:
	
	// Свойства для работы с файлами.
	bool canOpenInputFile = false;
	std::string inputFileName;
	std::string outputFileName;
	std::ofstream outputFileStream;

	// Поличество возможных потоков.
	size_t hardwareConcurrency;

	// Половина количества возможных потоков.
	size_t halfhardwareConcurrency;

	// Размер блока в файла в байтах.
	size_t sizeBlock;

	// Размер файла в байтах.
	uint64_t sizeFile;

	// Мьютех для работы потоков с файлом на запись.
	std::mutex mtxForWrite;

	// Вектор созданых потоков
	std::vector<std::thread> threads;

	// Закончил ли свою работу читатель.
	bool finishReader = false;

	// Вектор для работы с очередями блоков потоков обработчиков.
	std::vector<std::shared_ptr<CyclicQueue<FileBlock>>> cyclicQueue;

	// Метод потока читателя и метод потоков обработчиков.
	void threadReader();
	void threadHasher(size_t idThread);
	
	// Метод потоков обработчиков (читают, хэшируют, записывают).
	void threadReadHashWrite(size_t idThread);

	// Метод проверки возможности работы с потоком ввода
	bool checkInputFile();

	// Метод проверки на потенциальные исключения свойств класса
	void checkException();

	// Метод открытия потока записи
	void openOutputFile();

	// Метод записи строки хэша в файл
	void writeHashToFile(const std::int64_t&, const std::string&, const size_t&);
};

