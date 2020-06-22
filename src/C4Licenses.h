#ifndef C4LICENSES_H
#define C4LICENSES_H

#include<string>
#include<vector>

struct OCLicenseInfo { std::string path; std::string name; std::string content; };
extern const std::vector<OCLicenseInfo> OCLicenses;

#endif // C4LICENSES_H
