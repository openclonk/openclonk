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
static const std::wstring SAFE_CHARS = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-',.() ";

int main(int argc, char ** argv) {
	if (argc < 4 || argc % 2 != 0) {
		std::cerr << (argc > 0 ? argv[0] : "<null>") << ": Error: expected at least three arguments: one header output path and several pairs of (text file path, display name)" << std::endl;
		return -1;
	}
	std::vector<std::pair<char *, char *>> inputs;
	for (int i = 2; i < argc; i += 2)
		inputs.push_back(std::make_pair(argv[i], argv[i+1]));

	std::wofstream output(argv[1]);
	output << "#include<C4Licenses.h>" << std::endl
		   << "const std::vector<OCLicenseInfo> OCLicenses {" << std::endl;

	for(const auto& input : inputs) {
		output << std::endl << "\t{ \"" << input.first << "\", \"" << input.second << "\", std::string()" << std::endl;
		std::wifstream input_file(input.first);
		input_file.imbue(std::locale("C.UTF-8"));
		std::wstring line;
		while (std::getline(input_file, line)) {
			std::wstringstream out;
			for (const auto c : line) {
				if (SAFE_CHARS.find(c) != std::wstring::npos)
					out << c;
				else
					out << "\\U" << std::setfill(L'0') << std::setw(8) << std::right << std::hex << +c;
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
