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
#include <cstdlib>

// Generating this file in a way that neither bad old clang, bad old xcode, or bad old msvc will choke on it is actually not that easy.
// MSVC limits the string length to 16K, but just doing ("x" "y") or ("x" + "y") doesn't constitute two strings
// Clang on the other hand really doesn't like it if you have too many std::string() + std::string + … in one expression
// Old clang also seems to have a broken regex engine, with \.hpp|\.h and \.h|\.hpp not matching the same set of strings
// MSVCs regex engine, on the other hand, doesn't like long strings/complicated regexes: "regex_error(error_stack): There was insufficient memory to determine whether the regular expression could match the specified character sequence."
static const size_t maxblock = 16000;

// iswalnum will backstab
static const std::string SAFE_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-',.() ";

static std::string self = "<null>";
static std::string target = "";

[[noreturn]] void bail(std::string err) {
	std::string msg = self + ": ";
	if (target != "")
		msg += "in '" + target + "': ";
	msg += err;
	std::cerr << msg << std::endl;
	// Somehow Appveyor/msvc won't show stderr messages
	if (std::getenv("APPVEYOR") != nullptr)
		system(("appveyor AddCompilationMessage -Category Error \"" + msg + "\"").c_str());
	exit(-1);
}

std::string extract_comment(std::string content) {
	content = content.substr(content.find("/*")).substr(2);
	auto firstcommentend = content.find("*/");
	if (firstcommentend == content.npos)
		bail("No comment found (expecting a comment with a license)");
	else
		content = content.substr(0, firstcommentend);
	std::string upper(content);
	std::transform(content.begin(), content.end(), upper.begin(), toupper);
	if (upper.find("COPYRIGHT") == content.npos)
		bail("Found comment, but comment did not contain word 'copyright' - mimimimi");
	content = std::regex_replace(content, std::regex("^[ \t\n]*(\\* ?)?"), "");
	content = std::regex_replace(content, std::regex("\n \\* ?"), "\n");
	return content;
}
std::string extract_readme(std::string content) {
	std::string license_caption = "License\n-------\n";
	auto license_start = content.find(license_caption);
	if (license_start == content.npos)
		bail("No 'License' header");
	content = content.substr(license_start + license_caption.length());
	while (content[0] == '\n')
		content = content.substr(1);
	auto license_end = content.find("\n---");
	if (license_end != content.npos)
		content = content.substr(0, content.rfind("\n", license_end));
	return content;
}

std::string lendings(std::string content) {
	while (true) {
		auto pos = content.find("\r\n");
		if (pos == content.npos)
			break;
		content = content.substr(0, pos) + content.substr(pos + 1);
	}
	std::replace(content.begin(), content.end(), '\r', '\n');
	return content;
}

void mein(int argc, char ** argv) {
	if (argc > 0)
		self = argv[0];
	if (argc < 4 || argc % 2 != 0)
		bail("Error: expected at least three arguments: one header output path and several pairs of (text file path, display name)");
	std::vector<std::pair<char *, char *>> inputs;
	for (int i = 2; i < argc; i += 2)
		inputs.push_back(std::make_pair(argv[i], argv[i+1]));

	std::ofstream output(argv[1]);
	output << "#include<C4Licenses.h>" << std::endl
		   << "const std::vector<OCLicenseInfo> OCLicenses {" << std::endl;

	for(const auto& input : inputs) {
		target = input.first;
		std::ifstream input_file(input.first);
		input_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		std::stringstream buf;
		buf << input_file.rdbuf();
		buf.str(lendings(buf.str()));
		if (std::regex_match(input.first, std::regex(".*COPYING|.*TRADEMARK|licenses/.*|.*LICENSE(\\.txt)?")));
		else if (std::regex_match(input.first, std::regex(".*(\\.hpp|\\.h)")))
			buf.str(extract_comment(buf.str()));
		else if (std::regex_match(input.first, std::regex(".*/README\\.rst")))
			buf.str(extract_readme(buf.str()));
		else
			bail(std::string("No rule for treating ") + input.first);
		
		output << std::endl << "\t{ \"" << input.first << "\", \"" << input.second << "\", std::string(" << std::endl;
		std::string line;
		size_t block = 0;
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
			block += out.tellp();
			if (block > maxblock) {
				output << "\t\t) + std::string(" << std::endl;
				block = out.tellp();
			}
			output << "\t\t\"" << out.str() << "\\n\"" << std::endl;
		}
		output << "\t)}," << std::endl;
		target = "";
	}

	output << std::endl << "};" << std::endl;
}

int main(int argc, char ** argv) {
	try {
		mein(argc, argv);
	} catch(std::exception e) {
		bail(e.what());
	}
	return 0;
}
