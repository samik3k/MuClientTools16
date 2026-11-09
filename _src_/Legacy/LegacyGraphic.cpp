#include "LegacyGraphic.h"

#include <algorithm>
#include <cstdio>
#include <sstream>

namespace Legacy
{
	bool IsImageExt(const std::string& e)
	{
		return e == ".ozj" || e == ".ozt" || e == ".ozb" || e == ".ozp" || e == ".ozd" || e == ".ozg";
	}

	static bool IsJpeg(const uint8_t* p, size_t n) { return n >= 3 && p[0] == 0xFF && p[1] == 0xD8 && p[2] == 0xFF; }
	static bool IsPng(const uint8_t* p, size_t n) { return n >= 8 && std::memcmp(p, "\x89PNG\r\n\x1a\n", 8) == 0; }
	static bool IsDds(const uint8_t* p, size_t n) { return n >= 4 && std::memcmp(p, "DDS ", 4) == 0; }
	static bool IsBmp(const uint8_t* p, size_t n) { return n >= 54 && p[0] == 'B' && p[1] == 'M'; }

	// Windows .ico / .cur: reserved 0, type 1 or 2, image count 1..64
	static bool IsIco(const uint8_t* p, size_t n)
	{
		if (n < 22 || p[0] || p[1] || (p[2] != 1 && p[2] != 2) || p[3]) return false;
		uint16_t count = Get<uint16_t>(p + 4);
		return count >= 1 && count <= 64;
	}

	// Uncompressed true-colour TGA (what the client writes into OZT); returns pixel bytes needed or 0
	static size_t TgaPixelBytes(const uint8_t* p, size_t n)
	{
		if (n < 18 || p[1] != 0) return 0;
		uint16_t w = Get<uint16_t>(p + 12), h = Get<uint16_t>(p + 14);
		uint8_t bpp = p[16];
		if (!w || !h || (bpp != 24 && bpp != 32)) return 0;
		return (size_t)18 + p[0] + (size_t)w * h * (bpp / 8);
	}

	static bool LooksLikeTga(const uint8_t* p, size_t n)
	{
		size_t need = TgaPixelBytes(p, n);
		return need && need <= n && (p[2] == 2 || p[2] == 10 || p[2] == 0);
	}

	// 256x256 8-bit grey BMP (TerrainHeight) built from raw height bytes
	static Bytes MakeHeightBmp(const uint8_t* h)
	{
		Bytes b(54 + 1024 + 65536, 0);
		b[0] = 'B'; b[1] = 'M';
		Put<uint32_t>(&b[2], (uint32_t)b.size());
		Put<uint32_t>(&b[10], 54 + 1024);
		Put<uint32_t>(&b[14], 40);
		Put<int32_t>(&b[18], 256);
		Put<int32_t>(&b[22], 256);
		Put<uint16_t>(&b[26], 1);
		Put<uint16_t>(&b[28], 8);
		Put<uint32_t>(&b[34], 65536);
		Put<uint32_t>(&b[46], 256);
		for (int i = 0; i < 256; ++i) { b[54 + i * 4] = b[55 + i * 4] = b[56 + i * 4] = (uint8_t)i; }
		std::memcpy(&b[54 + 1024], h, 65536);
		return b;
	}

	bool UnpackImage(const fs::path& src, const fs::path& dstNoExt, std::string& msg)
	{
		Bytes d;
		if (!ReadFile(src, d)) { msg = "cannot read"; return false; }
		if (d.empty()) { msg = "empty file (0 bytes), skipped"; return false; }
		std::string ext = Lower(src.extension().string());
		const uint8_t* p = d.data();
		size_t n = d.size();
		std::string notes;

		auto out = [&](const char* e, const uint8_t* q, size_t len) {
			fs::path dst = dstNoExt;
			dst += e;
			if (!WriteFile(dst, q, len)) { msg = "cannot write " + dst.string(); return false; }
			msg = std::string(e + 1) + notes;
			return true;
		};

		// whole file is something else (icons stored as .ozt in 1.03/1.04)
		if (IsIco(p, n) && !(ext == ".ozt" && LooksLikeTga(p + 4, n - 4)))
		{
			notes = " (a Windows icon/cursor stored under " + ext + ")";
			return out(p[2] == 2 ? ".cur" : ".ico", p, n);
		}

		// TerrainHeight.OZB: the client skips 4 bytes and reads the 256x256 grid at file offset 1084, whatever the
		// BMP header says. Some files have a zeroed or junk header (or are 2 bytes short): rebuild those.
		if (ext == ".ozb" && (n == 66618 || n == 66620))
		{
			const uint8_t* q = p + 4;
			bool valid = q[0] == 'B' && q[1] == 'M' && Get<uint32_t>(q + 10) == 1078 && Get<int32_t>(q + 18) == 256 &&
				Get<int32_t>(q + 22) == 256 && Get<uint16_t>(q + 28) == 8;
			if (!valid)
			{
				Bytes grid(65536, 0);
				std::memcpy(grid.data(), p + 1084, n - 1084);
				notes = " (BMP header missing or invalid, rebuilt from the grid at offset 1084 as the client reads it";
				notes += n < 1084 + 65536 ? "; last 2 cells missing, set to 0)" : ")";
				Bytes b = MakeHeightBmp(grid.data());
				return out(".bmp", b.data(), b.size());
			}
		}

		static const size_t prefixes[] = { 24, 4, 0, 1 };
		for (size_t off : prefixes)
		{
			if (off >= n) continue;
			const uint8_t* q = p + off;
			size_t m = n - off;
			if (IsJpeg(q, m)) { if (ext != ".ozj") notes = " (JPEG data in " + ext + ")"; return out(".jpg", q, m); }
			if (IsPng(q, m)) { if (ext != ".ozp") notes = " (PNG data in " + ext + ")"; return out(".png", q, m); }
			if (IsDds(q, m)) { if (ext != ".ozd") notes = " (DDS data in " + ext + ")"; return out(".dds", q, m); }
			if (IsBmp(q, m) && Get<uint32_t>(q + 2) <= m + 8) { if (ext != ".ozb") notes = " (BMP data in " + ext + ")"; return out(".bmp", q, m); }
		}

		// TGA: .ozt has a 4-byte prefix; some .OZJ (1.01+) hold a 24-byte prefix + TGA
		for (size_t off : { (size_t)4, (size_t)24 })
		{
			if (off == 24 && ext != ".ozj") continue;
			if (off >= n || !LooksLikeTga(p + off, n - off)) continue;
			Bytes t(p + off, p + n);
			if (ext != ".ozt") notes = " (TGA data in " + ext + ")";
			if (t[2] == 0) { t[2] = 2; notes += " (TGA image type 0 repaired to 2)"; }
			return out(".tga", t.data(), t.size());
		}

		if (ext == ".ozg" && n > 4) { return out(".gfx", p + 4, n - 4); }

		msg = "unknown image data (first bytes " + Hex(p, n < 16 ? n : 16) + ")";
		return false;
	}

	bool IsModelFile(const Bytes& d)
	{
		return d.size() >= 4 && d[0] == 'B' && d[1] == 'M' && d[2] == 'D';
	}

	namespace
	{
		struct Reader
		{
			const Bytes& b;
			size_t p;
			bool ok = true;
			bool Need(size_t n) { if (p + n > b.size()) ok = false; return ok; }
			int16_t I16() { if (!Need(2)) return 0; int16_t v = Get<int16_t>(&b[p]); p += 2; return v; }
			uint8_t U8() { if (!Need(1)) return 0; return b[p++]; }
			std::string Str(size_t n)
			{
				if (!Need(n)) return "";
				std::string s((const char*)&b[p], strnlen((const char*)&b[p], n));
				p += n;
				return s;
			}
			void Skip(size_t n) { if (Need(n)) p += n; }
		};
	}

	bool UnpackModel(const fs::path& src, const fs::path& dstNoExt, std::string& msg)
	{
		Bytes d;
		if (!ReadFile(src, d)) { msg = "cannot read"; return false; }
		if (!IsModelFile(d)) { msg = "not a BMD model, and no table layout in the schema matches this file name"; return false; }
		uint8_t ver = d[3];
		Bytes body;
		std::string how;
		if (ver == 0x0A) { body.assign(d.begin() + 4, d.end()); how = "plain"; }
		else if (ver == 0x0C)
		{
			if (d.size() < 8) { msg = "truncated v0x0C header"; return false; }
			uint32_t size = Get<uint32_t>(&d[4]);
			if (size > d.size() - 8) { msg = "v0x0C size field larger than file"; return false; }
			body.assign(d.begin() + 8, d.begin() + 8 + size);
			MapFileDecrypt(body.data(), body.size());
			how = "map-file encrypted";
		}
		else if (ver == 0x00 && std::all_of(d.begin() + 3, d.end(), [](uint8_t c) { return c == 0; }))
		{
			msg = "empty placeholder model (\"BMD\" + zeros), skipped";
			return false;
		}
		else
		{
			char v[8];
			std::snprintf(v, sizeof v, "0x%02X", ver);
			msg = std::string("BMD version ") + v + " is not used by 0.68-1.04 clients (0x0E needs the Season 6+ tools)";
			return false;
		}

		Reader r{ body, 0 };
		std::ostringstream o;
		std::string name = r.Str(32);
		int16_t nm = r.I16(), nb = r.I16(), na = r.I16();
		o << "#MuLegacy\tmodel\tsource=" << src.filename().string() << "\tversion=0x" << std::hex << (int)ver << std::dec
			<< "\tencoding=" << how << "\n";
		o << "Name\t" << EscapeCell(name) << "\nMeshes\t" << nm << "\nBones\t" << nb << "\nActions\t" << na << "\n";
		if (nm < 0 || nb < 0 || na < 0 || !r.ok) { msg = "corrupt header"; return false; }

		o << "\n#Mesh\tVertices\tNormals\tTexCoords\tTriangles\tTextureId\tTexture\n";
		for (int i = 0; i < nm && r.ok; ++i)
		{
			int16_t nv = r.I16(), nn = r.I16(), nt = r.I16(), ntr = r.I16(), tex = r.I16();
			r.Skip((size_t)std::max<int16_t>(nv, 0) * 16 + (size_t)std::max<int16_t>(nn, 0) * 20 +
				(size_t)std::max<int16_t>(nt, 0) * 8 + (size_t)std::max<int16_t>(ntr, 0) * 64);
			std::string texName = r.Str(32);
			o << i << "\t" << nv << "\t" << nn << "\t" << nt << "\t" << ntr << "\t" << tex << "\t" << EscapeCell(texName) << "\n";
		}
		std::vector<int16_t> keys;
		o << "\n#Action\tKeys\tLockPositions\n";
		for (int i = 0; i < na && r.ok; ++i)
		{
			int16_t k = r.I16();
			uint8_t lock = r.U8();
			keys.push_back(k);
			if (lock) r.Skip((size_t)std::max<int16_t>(k, 0) * 12);
			o << i << "\t" << k << "\t" << (int)lock << "\n";
		}
		o << "\n#Bone\tName\tParent\n";
		for (int i = 0; i < nb && r.ok; ++i)
		{
			uint8_t dummy = r.U8();
			if (dummy) { o << i << "\t(dummy)\t\n"; continue; }
			std::string bn = r.Str(32);
			int16_t parent = r.I16();
			for (int16_t k : keys) r.Skip((size_t)std::max<int16_t>(k, 0) * 24);
			o << i << "\t" << EscapeCell(bn) << "\t" << parent << "\n";
		}
		if (!r.ok) { msg = "model data ends early (corrupt or unknown layout)"; return false; }
		if (r.p != body.size()) o << "\n# note: " << body.size() - r.p << " bytes after the last bone\n";

		fs::path txt = dstNoExt;
		txt += ".txt";
		if (!WriteText(txt, o.str())) { msg = "cannot write " + txt.string(); return false; }
		msg = "model, " + std::to_string(nm) + " meshes, " + std::to_string(nb) + " bones, " + std::to_string(na) + " actions";
		if (ver == 0x0C)
		{
			Bytes plain = { 'B', 'M', 'D', 0x0A };
			plain.insert(plain.end(), body.begin(), body.end());
			fs::path bmd = dstNoExt;
			bmd += ".bmd";
			if (!WriteFile(bmd, plain)) { msg = "cannot write " + bmd.string(); return false; }
			msg += " (decrypted copy written as BMD v0x0A)";
		}
		return true;
	}
}
