#include <iostream>
#include "TimeCode.hpp"
#include "Signature.h"
#include <CLI/CLI.hpp>


int main(int argc, char** argv)
{
	CLI::App app("File creation program", "CreateFile");
	std::string inputFileName = "C:\\Test_Signature\\Test_Signature\\InputFile2.bin";
	app.add_option("-i,--input", inputFileName, "Path to the filename.");
	std::string outputFileName = "C:\\Test_Signature\\Test_Signature\\OutputFile2.bin";
	app.add_option("-o,--output", outputFileName, "Path to the filename.");
	size_t sizeBlock = 1048576;
	app.add_option("-s, --bufferSize", sizeBlock, "Size of the buffer.");
	CLI11_PARSE(app, argc, argv);

	TimeCode tc;
	tc.getFirst();
	Signature signature(inputFileName, outputFileName, sizeBlock);
	if (signature.signatureFileSingleReader())
	{
		std::cout << "Signature file complited, time to work = " << tc.getLast() << '\n';
	}
	else
		std::cout << "fail suka\n";
	return 0;
}