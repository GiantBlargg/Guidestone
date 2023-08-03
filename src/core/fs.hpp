#pragma once

#include "types.hpp"
#include <filesystem>
#include <ranges>
#include <string>
#include <vector>

namespace FS {

class Reader {
  public:
	enum class MemoryType { Owned, Shared };
	const MemoryType memory_type;

	const u8* const data;
	const size_t size;

	size_t cursor = 0;
	std::endian endian = std::endian::little;

	Reader(MemoryType memory_type, const u8* data, size_t size);
	~Reader();

	Reader& operator>>(u8&);
	Reader& operator>>(u16&);
	Reader& operator>>(u32&);
	Reader& operator>>(i8& i) { return operator>>((u8&)i); }
	Reader& operator>>(i16& i) { return operator>>((u16&)i); }
	Reader& operator>>(i32& i) { return operator>>((u32&)i); }
	Reader& operator>>(f32& f) { return operator>>((u32&)f); }

	Reader& operator>>(std::string& str);

	template <std::ranges::range R> Reader& operator>>(R& r) {
		for (auto& o : r) {
			*this >> o;
		}
		return *this;
	}

	Reader& read(void* dest, size_t n);
	template <size_t n> Reader& operator>>(u8 (&d)[n]) { return read(d, n); }
	template <size_t n> Reader& operator>>(i8 (&d)[n]) { return read(d, n); }

	template <typename T> T get() {
		T t;
		*this >> t;
		return t;
	}
	template <typename T> T get(size_t pos) {
		cursor = pos;
		return get<T>();
	}
	template <typename T> std::vector<T> getVector(size_t n) {
		std::vector<T> t(n);
		*this >> t;
		return t;
	}
	template <typename T> std::vector<T> getVector(size_t n, size_t pos) {
		cursor = pos;
		return getVector<T>(n);
	}

	explicit operator bool() { return data != nullptr; }
};

using Path = std::filesystem::path;

Reader loadDataFile(const Path& filename);

Reader loadClassicFile(const Path& filename);

} // namespace FS
