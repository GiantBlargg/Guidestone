#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace fs {
std::vector<uint8_t> loadFile(std::string& filename);
}
