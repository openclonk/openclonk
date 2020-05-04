#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <utility>

static const auto ALNUM = std::string("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
static const char * delim = "=====#_#-#_#";

int main(int argc, char ** argv) {
	if (argc < 4 || argc % 2 != 0) {
		std::cerr << (argc > 0 ? argv[0] : "<null>") << ": Error: expected at least three arguments: one header output path and several pairs of (text file path, display name)" << std::endl;
		return -1;
	}
	std::vector<std::pair<std::string, std::string>> inputs;
	for (int i = 2; i < argc; i += 2)
		inputs.push_back(std::make_pair(argv[i], argv[i+1]));

	std::ofstream output(argv[1]);
	output << "#include<C4Licenses.h>" << std::endl
		   << "const std::vector<OCLicenseInfo> OCLicenses {" << std::endl;

	for(const auto& input : inputs)
		output << std::endl << "{ \"" << input.first << "\", \"" << input.second << "\", "
			   << "R\"" << delim << "(" << std::ifstream(input.first).rdbuf() << ")" << delim << "\" }," << std::endl;

	output << std::endl << "};" << std::endl;
	return 0;
}
