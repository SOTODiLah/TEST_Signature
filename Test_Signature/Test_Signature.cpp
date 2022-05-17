#include <iostream>
#include <exception>
#include "TimeCode.hpp"
#include "Signature.h"
#include "../CLI/CLI.hpp"


int main(int argc, char** argv) {
	CLI::App app("File creation signature", "CreateSignature");

	std::string inputFileName;
	app.add_option("-i,--input", inputFileName, "Path to the filename.");

	std::string outputFileName;
	app.add_option("-o,--output", outputFileName, "Path to the filename.");

	const size_t sizeBlock1mb = 1048576;
	size_t sizeBlock = sizeBlock1mb;
	app.add_option("-s, --sizeBuffer", sizeBlock, "Size of the buffer.");

	bool printTime = false;
	app.add_option("-t, --timeWork", printTime, "Print time work.");

	int method = 0;
	app.add_option("-m, --method", method, "Take method of threading.\n'0' - single reader;\n'1' - every thread is reader.");

	CLI11_PARSE(app, argc, argv);

	TimeCode tc;
	tc.getFirst();
	Signature signature(inputFileName, outputFileName, sizeBlock);
	try {
		if (method == 0)
			signature.signatureFileSingleReader();
		else if (method == 1)
			signature.signatureFileAllReader();
		else
			throw std::exception("Number of method is not correct.");

		std::cout << "Signature file complited";
		if (printTime)
			std::cout << ", time to work = " << tc.getLast()/1000.f << " seconds" << '\n';
		else
			std::cout << '\n';
	}
	catch (std::exception exp){
		std::cout << exp.what() << '\n';
	}

	return 0;
}
