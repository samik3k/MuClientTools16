#include "LegacyTerrain.h"

#include <cstdio>
#include <map>
#include <sstream>

// Formats (0.68 - 1.04, all 256x256, row-major, index = y * 256 + x):
//  ATT  65539 / 131075  plain             u8 ver, u8 w=255, u8 h=255, cells (u8 or u16)
//       65539           FC CF AB xor only same header (0.97.04 Terrain%d.att)
//       65540 / 131076  MapFileDecrypt, then FC CF AB over the whole file; u8 ver, u8 map, u8 w, u8 h, cells
//  MAP  196609          plain             u8 ver, layer1[65536], layer2[65536], alpha[65536]
//       196610          MapFileDecrypt    u8 ver, u8 map, layers (a raw first byte of 0 means an already decrypted dump)
//  OBJ  plain           u8 ver, i16 count, count * 30 bytes
//       MapFileDecrypt  u8 ver, u8 map, i16 count, count * 30 bytes
//       record: i16 type, f32 pos[3], f32 angle[3], f32 scale
// The header map byte often differs from the World folder number and a few files have a corrupt version byte;
// neither is treated as an error (the rest of those files is valid).

namespace Legacy
{
	// also backup copies found in private-server packages: "EncTerrain19.att a", "EncTerrain7.attbak"
	static std::string TerrainKind(const std::string& e)
	{
		for (const char* k : { ".att", ".map", ".obj" })
			if (e.compare(0, 4, k) == 0 && (e.size() == 4 || e == std::string(k) + "bak" || e == std::string(k) + " a")) return k;
		return "";
	}

	bool IsTerrainExt(const std::string& e) { return !TerrainKind(e).empty(); }

	static std::string GridDump(const uint8_t* cells, size_t cellBytes)
	{
		std::ostringstream o;
		char buf[8];
		for (int y = 0; y < 256; ++y)
		{
			for (int x = 0; x < 256; ++x)
			{
				size_t i = (size_t)y * 256 + x;
				if (cellBytes == 2) std::snprintf(buf, sizeof buf, "%04x", Get<uint16_t>(cells + i * 2));
				else std::snprintf(buf, sizeof buf, "%02x", cells[i]);
				o << (x ? " " : "") << buf;
			}
			o << "\n";
		}
		return o.str();
	}

	// <name>.tta|pam|jbo (decrypted) and <name>.att|map|obj.txt (dump; the kind is kept in the name because
	// Terrain.att, Terrain.map and Terrain.obj share one folder)
	static bool Write2(const fs::path& dstNoExt, const char* rawExt, const Bytes& raw, const std::string& txt, std::string& msg)
	{
		static const std::map<std::string, std::string> kind = { { ".tta", ".att" }, { ".pam", ".map" }, { ".jbo", ".obj" } };
		fs::path a = dstNoExt, b = dstNoExt;
		a += rawExt;
		b += kind.at(rawExt) + ".txt";
		if (!WriteFile(a, raw) || !WriteText(b, txt)) { msg = "cannot write output"; return false; }
		return true;
	}

	static bool Att(const Bytes& d, const fs::path& src, const fs::path& dstNoExt, std::string& msg)
	{
		size_t n = d.size();
		Bytes b;
		std::string variant;
		size_t hdr = 0;
		if (n == 65539 || n == 131075)
		{
			b = d;
			if (b[0] == 0 && b[1] == 255 && b[2] == 255) variant = "plain";
			else
			{
				Xor3(b.data(), b.size());
				if (b[0] == 0 && b[1] == 255 && b[2] == 255) variant = "xor (FC CF AB)";
			}
			hdr = 3;
		}
		else if (n == 65540 || n == 131076)
		{
			b = d;
			MapFileDecrypt(b.data(), b.size());
			Xor3(b.data(), b.size());
			if (b[2] == 255 && b[3] == 255) variant = "map-file encrypted + xor";
			hdr = 4;
		}
		if (variant.empty()) { msg = "unknown ATT layout (" + std::to_string(n) + " bytes)"; return false; }
		size_t cell = (n - hdr) / 65536;
		std::ostringstream o;
		o << "#MuLegacy\tterrain-att\tsource=" << src.filename().string() << "\tencoding=" << variant << "\n";
		o << "Version\t" << (int)b[0] << "\n";
		if (hdr == 4) o << "Map\t" << (int)b[1] << "\n";
		o << "Width\t" << (int)b[hdr - 2] << "\nHeight\t" << (int)b[hdr - 1] << "\nCellBytes\t" << cell << "\n";
		o << "# 256 rows (y = 0..255) of 256 hex cells (x = 0..255); cell = walk/safe-zone/no-move flags\n";
		o << GridDump(&b[hdr], cell);
		if (!Write2(dstNoExt, ".tta", b, o.str(), msg)) return false;
		msg = "ATT, " + variant + ", " + std::to_string(cell) + "-byte cells";
		return true;
	}

	static bool Map(const Bytes& d, const fs::path& src, const fs::path& dstNoExt, std::string& msg)
	{
		size_t n = d.size();
		Bytes b = d;
		std::string variant;
		size_t hdr;
		if (n == 196609) { variant = "plain"; hdr = 1; }
		else if (n == 196610)
		{
			hdr = 2;
			if (d[0] == 0) variant = "plain (already decrypted)";
			else
			{
				MapFileDecrypt(b.data(), b.size());
				variant = "map-file encrypted";
				if (b[0] != 0) variant += ", corrupt version byte";
			}
		}
		else { msg = "unknown MAP layout (" + std::to_string(n) + " bytes)"; return false; }
		std::ostringstream o;
		o << "#MuLegacy\tterrain-map\tsource=" << src.filename().string() << "\tencoding=" << variant << "\n";
		o << "Version\t" << (int)b[0] << "\n";
		if (hdr == 2) o << "Map\t" << (int)b[1] << "\n";
		const char* names[3] = { "Layer1 (tile texture index)", "Layer2 (tile texture index, 255 = none)", "Alpha (layer2 blend, value/255)" };
		for (int l = 0; l < 3; ++l)
			o << "\n# " << names[l] << ": 256 rows of 256 hex values\n" << GridDump(&b[hdr + (size_t)l * 65536], 1);
		if (!Write2(dstNoExt, ".pam", b, o.str(), msg)) return false;
		msg = "MAP, " + variant;
		return true;
	}

	static bool Obj(const Bytes& d, const fs::path& src, const fs::path& dstNoExt, std::string& msg)
	{
		size_t n = d.size();
		Bytes b = d;
		std::string variant;
		size_t hdr;
		if (n >= 3 && d[0] == 0 && 3 + 30 * (size_t)Get<uint16_t>(&d[1]) == n) { variant = "plain"; hdr = 3; }
		else
		{
			MapFileDecrypt(b.data(), b.size());
			if (n < 4 || 4 + 30 * (size_t)Get<uint16_t>(&b[2]) != n) { msg = "unknown OBJ layout (" + std::to_string(n) + " bytes)"; return false; }
			variant = "map-file encrypted";
			if (b[0] != 0) variant += ", corrupt version byte";
			hdr = 4;
		}
		size_t count = Get<uint16_t>(&b[hdr - 2]);
		std::ostringstream o;
		o << "#MuLegacy\tterrain-obj\tsource=" << src.filename().string() << "\tencoding=" << variant << "\n";
		o << "Version\t" << (int)b[0] << "\n";
		if (hdr == 4) o << "Map\t" << (int)b[1] << "\n";
		o << "Count\t" << count << "\n\n#\tType\tPosX\tPosY\tPosZ\tAngleX\tAngleY\tAngleZ\tScale\n";
		char buf[64];
		for (size_t i = 0; i < count; ++i)
		{
			const uint8_t* r = &b[hdr + i * 30];
			o << i << "\t" << Get<int16_t>(r);
			for (int k = 0; k < 7; ++k)
			{
				std::snprintf(buf, sizeof buf, "%.9g", Get<float>(r + 2 + k * 4));
				o << "\t" << buf;
			}
			o << "\n";
		}
		if (!Write2(dstNoExt, ".jbo", b, o.str(), msg)) return false;
		msg = "OBJ, " + variant + ", " + std::to_string(count) + " objects";
		return true;
	}

	bool UnpackTerrain(const fs::path& src, const fs::path& dstNoExt, std::string& msg)
	{
		Bytes d;
		if (!ReadFile(src, d)) { msg = "cannot read"; return false; }
		if (d.empty()) { msg = "empty file (0 bytes), skipped"; return false; }
		std::string e = TerrainKind(Lower(src.extension().string()));
		if (e == ".att") return Att(d, src, dstNoExt, msg);
		if (e == ".map") return Map(d, src, dstNoExt, msg);
		return Obj(d, src, dstNoExt, msg);
	}

	bool IsKeyFile(const std::string& n) { return n == "enc1.dat" || n == "dec2.dat"; }

	// SimpleModulus key files: u16 0x1112, u32 file size, 3 arrays of W u32 (W = 4, or 16 in 0.75),
	// each u32[i] xor'd with a fixed table. Enc1 = modulus, encryption key, xor key; Dec2 = modulus, decryption key, xor key.
	bool UnpackKeyFile(const fs::path& src, const fs::path& dstNoExt, std::string& msg)
	{
		static const uint32_t fix[16] = {
			0x3F08A79B, 0xE25CC287, 0x93D27AB9, 0x20DEA7BF, 0x837A9BC7, 0x1FFEA89B, 0xE836579C, 0xD89A0924,
			0x9D8A73CF, 0x4DEA98CE, 0x4DC9EF0F, 0x2BAC890A, 0xF32AFE54, 0xD8902A1E, 0x3FDEF8F9, 0x5A827990 };
		Bytes d;
		if (!ReadFile(src, d)) { msg = "cannot read"; return false; }
		if (d.size() < 6 || Get<uint16_t>(d.data()) != 0x1112 || Get<uint32_t>(&d[2]) != d.size() || (d.size() - 6) % 48)
		{
			msg = "not a SimpleModulus key file";
			return false;
		}
		size_t w = (d.size() - 6) / 12;
		bool enc = Lower(src.filename().string()) == "enc1.dat";
		const char* names[3] = { "Modulus", enc ? "EncryptionKey" : "DecryptionKey", "XorKey" };
		std::ostringstream o;
		o << "#MuLegacy\tkeys\tsource=" << src.filename().string() << "\twidth=" << w << "\n";
		for (int a = 0; a < 3; ++a)
		{
			o << names[a];
			for (size_t i = 0; i < w; ++i) o << "\t" << (Get<uint32_t>(&d[6 + (a * w + i) * 4]) ^ fix[i]);
			o << "\n";
		}
		fs::path t = dstNoExt;
		t += ".txt";
		if (!WriteText(t, o.str())) { msg = "cannot write " + t.string(); return false; }
		msg = std::string(enc ? "encryption" : "decryption") + " keys, " + std::to_string(w) + " per array";
		return true;
	}
}
