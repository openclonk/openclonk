#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include <array>
#include <sstream>
#include <iomanip>
#include <locale>

// iswalnum will backstab
static const std::string SAFE_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-',.() ";

int main(int argc, char ** argv) {
	if (argc < 4 || argc % 2 != 0) {
		std::cerr << (argc > 0 ? argv[0] : "<null>") << ": Error: expected at least three arguments: one header output path and several pairs of (text file path, display name)" << std::endl;
		return -1;
	}
	std::vector<std::pair<char *, char *>> inputs;
	for (int i = 2; i < argc; i += 2)
		inputs.push_back(std::make_pair(argv[i], argv[i+1]));

	std::ofstream output(argv[1]);
	output << "#include<C4Licenses.h>" << std::endl
		   << "const std::vector<OCLicenseInfo> OCLicenses {" << std::endl;

	for(const auto& input : inputs) {
		output << std::endl << "\t{ \"" << input.first << "\", \"" << input.second << "\", std::string()" << std::endl;
		std::ifstream input_file(input.first);
		std::string line;
		while (std::getline(input_file, line)) {
			std::stringstream out;
			for (const auto c : line) {
				if (SAFE_CHARS.find(c) != std::string::npos)
					out << c;
				else
					out << "\\" << std::setfill('0') << std::setw(3) << std::right << std::oct << +uint8_t(c);
			}
			output << "\t\t+ std::string(\"" << out.str() << "\\n\")" << std::endl;
		}
		if (!input_file.eof()) {
			std::cerr << "Failure reading " << input.first << std::endl;
		}
		output << "\t}," << std::endl;
	}

	output << std::endl << "};" << std::endl;
	return 0;
}
