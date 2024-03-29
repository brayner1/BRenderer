#include "FilesUtils.h"

#include <fstream>

namespace brr::files
{
	std::vector<char> ReadFile(const std::string& file_path)
	{
		std::ifstream file(file_path, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}
}
