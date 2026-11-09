// Shared helpers for the legacy (Season 0 - Season 4, client 0.68 - 1.04) readers.
// Portable C++17: no Windows headers, fixed-width types only, so it also builds with clang/gcc.
#ifndef LEGACY_COMMON_H
#define LEGACY_COMMON_H

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace Legacy
{
	namespace fs = std::filesystem;
	using Bytes = std::vector<uint8_t>;

	bool ReadFile(const fs::path& path, Bytes& out);
	bool WriteFile(const fs::path& path, const void* data, size_t size);
	inline bool WriteFile(const fs::path& path, const Bytes& b) { return WriteFile(path, b.data(), b.size()); }
	bool WriteText(const fs::path& path, const std::string& text);

	std::string Lower(std::string s);
	bool GlobMatch(const std::string& pattern, const std::string& name); // '*' and '?', case-insensitive
	std::string Hex(const uint8_t* p, size_t n);
	bool ParseHex(const std::string& s, Bytes& out);
	std::string Trim(const std::string& s);
	std::vector<std::string> Split(const std::string& s, char sep);

	// "BuxConvert": xor with FC CF AB, key index starting at 0
	void Xor3(uint8_t* p, size_t n);
	// MuCrypto::MapFileDecrypt / MapFileEncrypt (terrain files, BMD v0x0C models)
	void MapFileDecrypt(uint8_t* p, size_t n);
	void MapFileEncrypt(uint8_t* p, size_t n);
	// MuCrypto::CalculateCRC
	uint32_t CalculateCRC(const uint8_t* p, size_t n, uint16_t wkey);

	template <class T> inline T Get(const uint8_t* p) { T v; std::memcpy(&v, p, sizeof(T)); return v; }
	template <class T> inline void Put(uint8_t* p, T v) { std::memcpy(p, &v, sizeof(T)); }

	// Tab-separated text: escape \t \n \r \0 and backslash inside cells
	std::string EscapeCell(const std::string& s);
	std::string UnescapeCell(const std::string& s);
}

#endif // !LEGACY_COMMON_H
