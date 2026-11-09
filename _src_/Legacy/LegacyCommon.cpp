#include "LegacyCommon.h"

#include <algorithm>
#include <cctype>
#include <fstream>

namespace Legacy
{
	bool ReadFile(const fs::path& path, Bytes& out)
	{
		std::ifstream f(path, std::ios::binary);
		if (!f) return false;
		f.seekg(0, std::ios::end);
		std::streamoff size = f.tellg();
		if (size < 0) return false;
		f.seekg(0, std::ios::beg);
		out.resize((size_t)size);
		if (size > 0) f.read((char*)out.data(), size);
		return (bool)f;
	}

	bool WriteFile(const fs::path& path, const void* data, size_t size)
	{
		std::error_code ec;
		if (path.has_parent_path()) fs::create_directories(path.parent_path(), ec);
		std::ofstream f(path, std::ios::binary | std::ios::trunc);
		if (!f) return false;
		if (size) f.write((const char*)data, size);
		return (bool)f;
	}

	bool WriteText(const fs::path& path, const std::string& text)
	{
		return WriteFile(path, text.data(), text.size());
	}

	std::string Lower(std::string s)
	{
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
		return s;
	}

	static bool GlobRec(const char* p, const char* s)
	{
		for (; *p; ++p, ++s)
		{
			if (*p == '*')
			{
				for (const char* t = s; ; ++t)
				{
					if (GlobRec(p + 1, t)) return true;
					if (!*t) return false;
				}
			}
			if (!*s) return false;
			if (*p != '?' && std::tolower((unsigned char)*p) != std::tolower((unsigned char)*s)) return false;
		}
		return *s == 0;
	}

	bool GlobMatch(const std::string& pattern, const std::string& name)
	{
		return GlobRec(pattern.c_str(), name.c_str());
	}

	std::string Hex(const uint8_t* p, size_t n)
	{
		static const char* d = "0123456789abcdef";
		std::string s(n * 2, '0');
		for (size_t i = 0; i < n; ++i) { s[2 * i] = d[p[i] >> 4]; s[2 * i + 1] = d[p[i] & 15]; }
		return s;
	}

	bool ParseHex(const std::string& s, Bytes& out)
	{
		if (s.size() % 2) return false;
		out.clear();
		for (size_t i = 0; i < s.size(); i += 2)
		{
			auto nib = [](char c) -> int {
				if (c >= '0' && c <= '9') return c - '0';
				c = (char)std::tolower((unsigned char)c);
				if (c >= 'a' && c <= 'f') return c - 'a' + 10;
				return -1;
			};
			int a = nib(s[i]), b = nib(s[i + 1]);
			if (a < 0 || b < 0) return false;
			out.push_back((uint8_t)(a * 16 + b));
		}
		return true;
	}

	std::string Trim(const std::string& s)
	{
		size_t a = s.find_first_not_of(" \t\r\n");
		if (a == std::string::npos) return "";
		size_t b = s.find_last_not_of(" \t\r\n");
		return s.substr(a, b - a + 1);
	}

	std::vector<std::string> Split(const std::string& s, char sep)
	{
		std::vector<std::string> v;
		size_t a = 0;
		for (;;)
		{
			size_t b = s.find(sep, a);
			v.push_back(s.substr(a, b == std::string::npos ? std::string::npos : b - a));
			if (b == std::string::npos) return v;
			a = b + 1;
		}
	}

	void Xor3(uint8_t* p, size_t n)
	{
		static const uint8_t key[3] = { 0xFC, 0xCF, 0xAB };
		for (size_t i = 0; i < n; ++i) p[i] ^= key[i % 3];
	}

	static const uint8_t kMapKey[16] = { 0xD1, 0x73, 0x52, 0xF6, 0xD2, 0x9A, 0xCB, 0x27,
	                                     0x3E, 0xAF, 0x59, 0x31, 0x37, 0xB3, 0xE7, 0xA2 };

	void MapFileDecrypt(uint8_t* p, size_t n)
	{
		uint8_t seed = 0x5E;
		for (size_t i = 0; i < n; ++i)
		{
			uint8_t enc = p[i];
			p[i] = (uint8_t)((enc ^ kMapKey[i % 16]) - seed);
			seed = (uint8_t)(enc + 0x3D);
		}
	}

	void MapFileEncrypt(uint8_t* p, size_t n)
	{
		uint8_t seed = 0x5E;
		for (size_t i = 0; i < n; ++i)
		{
			p[i] = (uint8_t)((uint8_t)(p[i] + seed) ^ kMapKey[i % 16]);
			seed = (uint8_t)(p[i] + 0x3D);
		}
	}

	uint32_t CalculateCRC(const uint8_t* p, size_t n, uint16_t wkey)
	{
		uint32_t crc = (uint32_t)wkey << 9;
		for (size_t i = 0; i + 4 <= n; i += 4)
		{
			uint32_t t = Get<uint32_t>(p + i);
			if ((wkey + (i >> 2)) % 2 == 1) crc += t;
			else crc ^= t;
			if (i % 16 == 0) crc ^= (crc + wkey) >> ((i >> 2) % 8 + 1);
		}
		return crc;
	}

	std::string EscapeCell(const std::string& s)
	{
		std::string r;
		r.reserve(s.size());
		for (char c : s)
		{
			switch (c)
			{
			case '\t': r += "\\t"; break;
			case '\n': r += "\\n"; break;
			case '\r': r += "\\r"; break;
			case '\\': r += "\\\\"; break;
			case '\0': r += "\\0"; break;
			default: r += c;
			}
		}
		return r;
	}

	std::string UnescapeCell(const std::string& s)
	{
		std::string r;
		r.reserve(s.size());
		for (size_t i = 0; i < s.size(); ++i)
		{
			if (s[i] == '\\' && i + 1 < s.size())
			{
				char n = s[++i];
				r += n == 't' ? '\t' : n == 'n' ? '\n' : n == 'r' ? '\r' : n == '0' ? '\0' : n;
			}
			else r += s[i];
		}
		return r;
	}
}
