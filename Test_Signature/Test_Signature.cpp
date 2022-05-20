#include <iostream>
#include <exception>
#include "Signature.hpp"
#include "Other/Timers.hpp"
#include "../CLI/CLI.hpp"


int main(int argc, char** argv) 
{
	try
	{
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

		Signature signature;
		{
			Timers::AutoTimer timer("working");
			signature.generate(inputFileName, outputFileName, sizeBlock);
		}
	}
	catch (const std::exception& exp) { std::cout << exp.what() << '\n'; }
	catch (const std::string& exp) { std::cout << exp.c_str() << '\n'; }
	catch (const char* exp) { std::cout << exp << '\n'; }
	catch (...) { std::cout << "unknown exception" << '\n'; }

	return 0;
}
