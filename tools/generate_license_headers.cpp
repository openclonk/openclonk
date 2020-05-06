#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include <array>
#include <sstream>
#include <iomanip>
#include <locale>
#include <regex>

// iswalnum will backstab
static const std::string SAFE_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-',.() ";

static std::string self = "<null>";

std::string extract_comment(std::string name, std::string content) {
	std::regex licensecomment("^[ \t\n]*/\\*(.|\r|\n)*?copyright(.|\r|\n)*? \\*/", std::regex_constants::icase);
	std::smatch m;
	if (!std::regex_search(content, m, licensecomment)) {
		std::cerr << "Initial comment containing license not found in " << name << std::endl;
		exit(-3);
	}
	std::string dc(m[0]);
	dc = std::regex_replace(dc, std::regex("^[ \t\n]*/\\*[ \t\r\n]*(\\* ?)?"), "");
	dc = std::regex_replace(dc, std::regex("\r?\n \\* "), "\n");
	dc = std::regex_replace(dc, std::regex("\r?\n \\*(\r?\n)"), "\n\n");
	dc = std::regex_replace(dc, std::regex("\r?\n \\*/.*$"), "");
	return dc;
}
std::string extract_readme(std::string name, std::string content) {
	std::regex license("^(.|\n|\r)*?License(\n|\r)+-------(\n|\r){2,4}((.|\n|\r)+?)($|.+\r?\n---+(.|\r|\n)*$)");
	std::smatch m;
	if (!std::regex_search(content, m, license)) {
		std::cerr << "License section not found in " << name << std::endl;
		exit(-4);
	}
	return m[4];
}

int main(int argc, char ** argv) {
	if (argc > 0)
		self = argv[0];
	if (argc < 4 || argc % 2 != 0) {
		std::cerr << self << ": Error: expected at least three arguments: one header output path and several pairs of (text file path, display name)" << std::endl;
		exit(-1);
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
		input_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		std::stringstream buf;
		buf << input_file.rdbuf();
		if (regex_match(input.first, std::regex(".*COPYING|.*TRADEMARK|licenses/.*|.*LICENSE(\\.txt)?")));
		else if (regex_match(input.first, std::regex(".*(\\.h|\\.hpp)")))
			buf.str(extract_comment(input.first, buf.str()));
		else if (regex_match(input.first, std::regex(".*/README\\.rst")))
			buf.str(extract_readme(input.first, buf.str()));
		else {
			std::cerr << self << ": No rule for treating " << input.first << std::endl;
			exit(-2);
		}
		
		std::string line;
		while (std::getline(buf, line)) {
			std::stringstream out;
			for (const auto c : line) {
				if (SAFE_CHARS.find(c) != std::string::npos)
					out << c;
				else
					out << "\\" << std::setfill('0') << std::setw(3) << std::right << std::oct << +uint8_t(c);
			}
			if (line.empty())
				out << " ";
			output << "\t\t+ std::string(\"" << out.str() << "\\n\")" << std::endl;
		}
		output << "\t}," << std::endl;
	}

	output << std::endl << "};" << std::endl;
	return 0;
}
