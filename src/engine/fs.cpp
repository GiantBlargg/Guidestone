#include "fs.hpp"

#include <fstream>

namespace fs {
std::vector<uint8_t> loadFile(std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	std::vector<uint8_t> buffer(file.tellg());
	file.seekg(0);
	file.read((char*)buffer.data(), buffer.size());
	return buffer;
}
} // namespace fs
