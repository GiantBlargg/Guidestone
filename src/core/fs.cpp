#include "fs.hpp"

#include "log.hpp"
#include <cstring>
#include <fstream>

namespace FS {
Reader::Reader(MemoryType memory_type, const u8* data, size_t size)
	: memory_type(memory_type), data(data), size(size) {}

Reader::~Reader() {
	if (memory_type == MemoryType::Owned)
		delete[] data;
}

Reader& Reader::operator>>(u8& d) { return read(&d, 1); }

static_assert(
	std::endian::native == std::endian::little || std::endian::native == std::endian::big,
	"Mixed Endian not supported!");

Reader& Reader::operator>>(u16& d) {
	if (endian == std::endian::native) {
		return read((u8*)&d, 2);
	} else {
		u8* _d = (u8*)&d;
		return *this >> _d[1] >> _d[0];
	}
}

Reader& Reader::operator>>(u32& d) {
	if (endian == std::endian::native) {
		return read((u8*)&d, 4);
	} else {
		u8* _d = (u8*)&d;
		return *this >> _d[3] >> _d[2] >> _d[1] >> _d[0];
	}
}

Reader& Reader::operator>>(std::string& str) {
	size_t len = strnlen((const char*)(data + cursor), size - cursor);
	str = std::string((const char*)(data + cursor), len);
	cursor += len + 1;
	return *this;
}

Reader& Reader::read(void* dest, size_t n) {
	if (cursor + n <= size) [[likely]] {
		memcpy(dest, data + cursor, n);
		cursor += n;
	} else [[unlikely]] {
		size_t _n = size - cursor;
		Log::error("Read past end");
		memcpy(dest, data + cursor, _n);
		cursor = size;
	}
	return *this;
}

Reader loadRealFile(Path filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file) {
		Log::error("File " + filename.string() + " could not be opened");
		return Reader(Reader::MemoryType::Shared, nullptr, 0);
	}
	size_t size = file.tellg();
	u8* buffer = new u8[size];
	file.seekg(0);
	file.read((char*)buffer, size);
	return Reader(Reader::MemoryType::Owned, buffer, size);
}

Reader loadDataFile(const Path& filename) { return loadRealFile(filename); }

Reader loadClassicFile(const Path& filename) {
	static const Path classic_path = getenv("HWC_DATA");
	return loadRealFile(classic_path / filename);
}
} // namespace FS
