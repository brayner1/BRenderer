#ifndef BRR_FILESUTILS_H
#define BRR_FILESUTILS_H
#include <string>
#include <vector>

namespace brr::files
{

	std::vector<char> ReadFile(const std::string& file_path);

}

#endif