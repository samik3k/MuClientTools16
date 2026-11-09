#include "LegacySchema.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <regex>
#include <sstream>

namespace Legacy
{
	static bool ParseType(const std::string& t, Field& f)
	{
		static const std::map<std::string, std::pair<FieldType, size_t>> simple = {
			{ "u8", { FieldType::U8, 1 } },   { "i8", { FieldType::I8, 1 } },
			{ "u16", { FieldType::U16, 2 } }, { "i16", { FieldType::I16, 2 } },
			{ "u32", { FieldType::U32, 4 } }, { "i32", { FieldType::I32, 4 } },
			{ "f32", { FieldType::F32, 4 } },
		};
		auto it = simple.find(t);
		if (it != simple.end()) { f.type = it->second.first; f.size = it->second.second; return true; }
		std::smatch m;
		static const std::regex sized("(str|raw)([0-9]+)");
		if (std::regex_match(t, m, sized))
		{
			f.type = m[1] == "str" ? FieldType::Str : FieldType::Raw;
			f.size = (size_t)std::stoul(m[2]);
			return f.size > 0;
		}
		return false;
	}

	size_t Variant::FieldBytes() const
	{
		size_t n = 0;
		for (auto& f : fields) n += f.size * f.count;
		return n;
	}

	bool Schema::Load(const fs::path& path, std::string& err)
	{
		tables.clear();
		std::error_code ec;
		if (fs::is_directory(path, ec))
		{
			std::vector<fs::path> files;
			for (auto& e : fs::directory_iterator(path, ec))
				if (e.is_regular_file() && Lower(e.path().extension().string()) == ".txt") files.push_back(e.path());
			std::sort(files.begin(), files.end());
			for (auto& f : files) if (!LoadFile(f, err)) return false;
			return true;
		}
		return LoadFile(path, err);
	}

	bool Schema::LoadFile(const fs::path& path, std::string& err)
	{
		std::ifstream in(path);
		if (!in) { err = "cannot open schema " + path.string(); return false; }
		std::string line;
		int ln = 0;
		Table* cur = nullptr;
		Variant* var = nullptr;
		auto fail = [&](const std::string& what) {
			err = path.filename().string() + ":" + std::to_string(ln) + ": " + what + ": " + line;
			return false;
		};
		static const std::regex opt("(\\w+)=(\"[^\"]*\"|\\S+)");
		while (std::getline(in, line))
		{
			++ln;
			std::string s = line;
			size_t hash = s.find('#');
			if (hash != std::string::npos) s = s.substr(0, hash);
			s = Trim(s);
			if (s.empty()) continue;
			std::istringstream ss(s);
			std::string dir;
			ss >> dir;
			if (dir == "table")
			{
				Table t;
				std::string globs;
				ss >> t.name >> globs;
				if (t.name.empty() || globs.empty()) return fail("table needs <name> <globs>");
				t.globs = Split(globs, ',');
				tables.push_back(t);
				cur = &tables.back();
				var = nullptr;
			}
			else if (dir == "variant")
			{
				if (!cur) return fail("variant before table");
				Variant v;
				std::string sizes;
				ss >> v.id >> sizes;
				if (v.id.empty() || sizes.empty()) return fail("variant needs <id> <sizes>");
				if (sizes[0] == '=')
					for (auto& x : Split(sizes.substr(1), ',')) v.sizes.push_back((size_t)std::stoull(x));
				else if (sizes != "*") return fail("sizes must be =a,b,... or *");
				std::string rest;
				std::getline(ss, rest);
				for (std::sregex_iterator it(rest.begin(), rest.end(), opt), end; it != end; ++it)
				{
					std::string k = (*it)[1], val = (*it)[2];
					if (val.size() >= 2 && val.front() == '"') val = val.substr(1, val.size() - 2);
					if (k == "rec") v.rec = (size_t)std::stoul(val);
					else if (k == "header")
					{
						v.header = val;
						if (val.compare(0, 9, "counts32:") == 0) { v.header = "counts32"; v.headerCounts = (size_t)std::stoul(val.substr(9)); }
					}
					else if (k == "skip") v.skip = (size_t)std::stoul(val);
					else if (k == "probe")
					{
						auto p = Split(val, ':');
						if (p.size() != 3) return fail("probe must be <record>:<offset>:<value>");
						v.probeRec = std::stoll(p[0]); v.probeOff = (size_t)std::stoul(p[1]); v.probeVal = std::stoi(p[2]);
					}
					else if (k == "skipxor") v.skipXor = (size_t)std::stoul(val);
					else if (k == "xor") v.xorMode = val;
					else if (k == "crc") v.crcKey = val == "none" ? -1 : (int)std::stoul(val, nullptr, 16);
					else if (k == "crccheck") v.crcCheck = val != "no";
					else if (k == "crcscope") v.crcScope = val;
					else if (k == "magic") v.magic.assign(val.begin(), val.end());
					else if (k == "layout") v.layout = val;
					else if (k == "label") v.label = val;
					else return fail("unknown option " + k);
				}
				if (v.layout != "fixed" && v.layout != "text-indexed") return fail("unknown layout");
				if (v.header != "none" && v.header != "count32" && v.header != "count32cap" &&
					!(v.header == "counts32" && v.headerCounts > 0)) return fail("unknown header");
				if (v.xorMode != "record" && v.xorMode != "stream" && v.xorMode != "none" && v.xorMode != "kayito") return fail("unknown xor");
				if (v.crcScope != "all" && v.crcScope != "body") return fail("unknown crcscope");
				cur->variants.push_back(v);
				var = &cur->variants.back();
			}
			else if (dir == "field" || dir == "pad")
			{
				if (!var) return fail("field before variant");
				Field f;
				std::string t;
				if (dir == "pad") { f.pad = true; f.name = "_pad"; ss >> t; t = "raw" + t; }
				else ss >> f.name >> t;
				size_t star = t.find('*');
				if (star != std::string::npos) { f.count = (size_t)std::stoul(t.substr(star + 1)); t = t.substr(0, star); }
				if (!ParseType(t, f) || f.count == 0) return fail("bad field type");
				var->fields.push_back(f);
			}
			else return fail("unknown directive");
		}
		for (auto& t : tables)
			for (auto& v : t.variants)
				if (v.layout == "fixed" && v.FieldBytes() != v.rec)
				{
					err = t.name + "." + v.id + ": fields add up to " + std::to_string(v.FieldBytes()) +
						" bytes but rec=" + std::to_string(v.rec);
					return false;
				}
		return true;
	}

	const Table* Schema::FindTable(const std::string& fileName) const
	{
		for (auto& t : tables)
			for (auto& g : t.globs)
				if (GlobMatch(g, fileName)) return &t;
		return nullptr;
	}

	const Table* Schema::TableByName(const std::string& name) const
	{
		for (auto& t : tables) if (t.name == name) return &t;
		return nullptr;
	}

	static size_t Trailer(const Variant& v) { return v.crcKey >= 0 ? 4 : 0; }
	static size_t HeaderBytes(const Variant& v)
	{
		return v.skip + (v.header == "none" ? 0 : v.header == "counts32" ? 4 * v.headerCounts : 4);
	}

	static void XorPeriod(uint8_t* p, size_t n, size_t period)
	{
		if (!period) return;
		for (size_t i = 0; i < n; i += period) Xor3(p + i, std::min(period, n - i));
	}

	// counts32: the T counts, xor'd as one block
	static std::vector<int32_t> HeaderCounts(const Variant& v, const Bytes& d)
	{
		Bytes b(d.begin() + v.skip, d.begin() + v.skip + 4 * v.headerCounts);
		Xor3(b.data(), b.size());
		std::vector<int32_t> c;
		for (size_t i = 0; i < v.headerCounts; ++i) c.push_back(Get<int32_t>(&b[i * 4]));
		return c;
	}

	// number of records announced by the header (-1 = no count in the header)
	static long long RecordCount(const Variant& v, const Bytes& d)
	{
		if (v.header == "none") return -1;
		if (v.header == "counts32")
		{
			long long s = 0;
			for (int32_t c : HeaderCounts(v, d)) { if (c < 0) return -2; s += c; }
			return s;
		}
		return Get<uint32_t>(&d[v.skip]);
	}

	// where the CRC starts: everything before the trailer, or only the records after a count header
	static size_t CrcStart(const Variant& v) { return v.crcScope == "body" ? HeaderBytes(v) : 0; }

	static bool CrcMatches(const Variant& v, const Bytes& d)
	{
		if (v.crcKey < 0 || !v.crcCheck) return true;
		size_t s = CrcStart(v);
		if (d.size() < s + 4) return false;
		return CalculateCRC(d.data() + s, d.size() - 4 - s, (uint16_t)v.crcKey) == Get<uint32_t>(&d[d.size() - 4]);
	}

	// kayito's ServerInfo.bmd cipher: plain[i] = enc[i] ^ K[i % 20] ^ (i & 0xFF)
	static void KayitoXor(uint8_t* p, size_t n)
	{
		static const uint8_t k[20] = { 0x9e, 0x98, 0x12, 0x2b, 0xb6, 0x76, 0xf6, 0x5b, 0x76, 0x4d,
		                               0x08, 0xf2, 0xab, 0xdf, 0x91, 0xb6, 0x4c, 0x1c, 0x93, 0x2f };
		for (size_t i = 0; i < n; ++i) p[i] ^= k[i % 20] ^ (uint8_t)(i & 0xFF);
	}

	static bool TextIndexedFits(const Bytes& d, size_t start, size_t trailer)
	{
		if (d.size() < start + 4 + trailer) return false;
		uint32_t n = Get<uint32_t>(d.data() + start);
		size_t p = start + 4, end = d.size() - trailer;
		for (uint32_t i = 0; i < n; ++i)
		{
			if (p + 8 > end) return false;
			uint32_t len = Get<uint32_t>(&d[p + 4]);
			p += 8;
			if (len > end - p) return false;
			p += len;
		}
		return p == end;
	}

	const Variant* Schema::Pick(const Table& t, const Bytes& d) const
	{
		for (auto& v : t.variants)
		{
			if (!v.sizes.empty() && std::find(v.sizes.begin(), v.sizes.end(), d.size()) == v.sizes.end()) continue;
			size_t tr = Trailer(v), hd = HeaderBytes(v);
			if (d.size() < tr + hd) continue;
			if (!CrcMatches(v, d)) continue;
			if (v.layout == "text-indexed")
			{
				if (d.size() < v.magic.size() || !std::equal(v.magic.begin(), v.magic.end(), d.begin())) continue;
				if (!TextIndexedFits(d, v.magic.size(), tr)) continue;
			}
			else
			{
				size_t body = d.size() - tr - hd;
				if (v.rec == 0) continue;
				long long cnt = RecordCount(v, d);
				if (cnt == -2) continue;
				if (v.header == "count32cap")
				{
					// count records, then unused slots of a fixed-size buffer (the client ignores them)
					if ((size_t)cnt * v.rec > body) continue;
				}
				else
				{
					if (body % v.rec) continue;
					if (cnt >= 0 && (size_t)cnt * v.rec != body) continue;
				}
				if (v.probeRec >= 0)
				{
					if (v.xorMode != "record" || v.probeOff >= v.rec || (size_t)(v.probeRec + 1) * v.rec > body) continue;
					Bytes r(d.begin() + hd + v.probeRec * v.rec, d.begin() + hd + (v.probeRec + 1) * v.rec);
					Xor3(r.data(), r.size());
					if (r[v.probeOff] != v.probeVal) continue;
				}
			}
			return &v;
		}
		return nullptr;
	}

	static std::string FormatValue(const Field& f, const uint8_t* p)
	{
		char buf[64];
		switch (f.type)
		{
		case FieldType::U8: return std::to_string(p[0]);
		case FieldType::I8: return std::to_string((int8_t)p[0]);
		case FieldType::U16: return std::to_string(Get<uint16_t>(p));
		case FieldType::I16: return std::to_string(Get<int16_t>(p));
		case FieldType::U32: return std::to_string(Get<uint32_t>(p));
		case FieldType::I32: return std::to_string(Get<int32_t>(p));
		case FieldType::F32:
		{
			// garbage floats (NaN payloads) would not survive text: write those as raw bits, e.g. 0x7fc00001
			float v = Get<float>(p);
			std::snprintf(buf, sizeof(buf), "%.9g", v);
			float back = std::strtof(buf, nullptr);
			if (std::memcmp(&back, p, 4) != 0) std::snprintf(buf, sizeof(buf), "0x%08x", (unsigned)Get<uint32_t>(p));
			return buf;
		}
		case FieldType::Str:
		{
			// keep leftover bytes after the terminator (shown as \0...) so packing gives back the same file
			size_t n = f.size;
			while (n && p[n - 1] == 0) --n;
			return EscapeCell(std::string((const char*)p, n));
		}
		case FieldType::Raw: return Hex(p, f.size);
		}
		return "";
	}

	static bool ParseValue(const Field& f, const std::string& cell, uint8_t* p, std::string& err)
	{
		try
		{
			switch (f.type)
			{
			case FieldType::U8: case FieldType::I8: p[0] = (uint8_t)std::stol(cell); return true;
			case FieldType::U16: case FieldType::I16: Put<uint16_t>(p, (uint16_t)std::stol(cell)); return true;
			case FieldType::U32: Put<uint32_t>(p, (uint32_t)std::stoul(cell)); return true;
			case FieldType::I32: Put<int32_t>(p, (int32_t)std::stol(cell)); return true;
			case FieldType::F32:
				if (cell.size() == 10 && cell[0] == '0' && (cell[1] == 'x' || cell[1] == 'X')) Put<uint32_t>(p, (uint32_t)std::stoul(cell, nullptr, 16));
				else Put<float>(p, std::strtof(cell.c_str(), nullptr));
				return true;
			case FieldType::Str:
			{
				std::string s = UnescapeCell(cell);
				if (s.size() > f.size) { err = f.name + ": text longer than " + std::to_string(f.size) + " bytes"; return false; }
				std::memset(p, 0, f.size);
				std::memcpy(p, s.data(), s.size());
				return true;
			}
			case FieldType::Raw:
			{
				Bytes b;
				if (!ParseHex(cell, b) || b.size() != f.size) { err = f.name + ": expected " + std::to_string(f.size * 2) + " hex digits"; return false; }
				std::memcpy(p, b.data(), b.size());
				return true;
			}
			}
		}
		catch (...) {}
		err = f.name + ": bad value '" + cell + "'";
		return false;
	}

	bool UnpackTable(const Schema& schema, const fs::path& src, const fs::path& dst, std::string& msg)
	{
		Bytes d;
		if (!ReadFile(src, d)) { msg = "cannot read"; return false; }
		const Table* t = schema.FindTable(src.filename().string());
		if (!t) { msg = "no table for this file name"; return false; }
		const Variant* v = schema.Pick(*t, d);
		if (!v) { msg = t->name + ": no variant matches size " + std::to_string(d.size()); return false; }

		std::ostringstream o;
		o << "#MuLegacy\ttable=" << t->name << "\tvariant=" << v->id << "\tsource=" << src.filename().string()
			<< "\tsize=" << d.size();
		if (!v->label.empty()) o << "\tlabel=" << v->label;
		if (v->crcKey >= 0 && !v->crcCheck) o << "\ttrailer=" << Hex(&d[d.size() - 4], 4);
		size_t used = 0; // records actually read (count32cap keeps the unused slots as "tail")
		if (v->layout == "fixed")
		{
			if (v->skip)
			{
				Bytes s(d.begin(), d.begin() + v->skip);
				XorPeriod(s.data(), s.size(), v->skipXor);
				o << "\tskip=" << Hex(s.data(), s.size());
			}
			if (v->header == "counts32")
			{
				o << "\tcounts=";
				auto c = HeaderCounts(*v, d);
				for (size_t i = 0; i < c.size(); ++i) o << (i ? "," : "") << c[i];
			}
			used = d.size() - Trailer(*v) - HeaderBytes(*v);
			if (v->header == "count32cap")
			{
				used = (size_t)RecordCount(*v, d) * v->rec;
				size_t tailStart = HeaderBytes(*v) + used, tailEnd = d.size() - Trailer(*v);
				o << "\ttail=" << Hex(d.data() + tailStart, tailEnd - tailStart);
			}
		}
		o << "\n";
		size_t rows = 0;

		if (v->layout == "text-indexed")
		{
			o << "#Index\tText\n";
			size_t p = v->magic.size();
			uint32_t n = Get<uint32_t>(&d[p]);
			p += 4;
			for (uint32_t i = 0; i < n; ++i, ++rows)
			{
				uint32_t idx = Get<uint32_t>(&d[p]), len = Get<uint32_t>(&d[p + 4]);
				p += 8;
				Bytes s(d.begin() + p, d.begin() + p + len);
				p += len;
				if (v->xorMode != "none") Xor3(s.data(), s.size());
				o << idx << "\t" << EscapeCell(std::string(s.begin(), s.end())) << "\n";
			}
		}
		else
		{
			o << "#";
			for (auto& f : v->fields)
			{
				if (f.pad) continue;
				for (size_t i = 0; i < f.count; ++i)
					o << "\t" << f.name << (f.count > 1 ? "[" + std::to_string(i) + "]" : "");
			}
			o << "\n";
			size_t start = HeaderBytes(*v), end = start + used;
			Bytes area(d.begin() + start, d.begin() + end);
			if (v->xorMode == "stream") Xor3(area.data(), area.size());
			if (v->xorMode == "kayito") KayitoXor(area.data(), area.size());
			for (size_t k = 0; k * v->rec < area.size(); ++k, ++rows)
			{
				uint8_t* r = &area[k * v->rec];
				if (v->xorMode == "record") Xor3(r, v->rec);
				o << k;
				size_t q = 0;
				for (auto& f : v->fields)
					for (size_t i = 0; i < f.count; ++i, q += f.size)
						if (!f.pad) o << "\t" << FormatValue(f, r + q);
				o << "\n";
			}
		}
		if (!WriteText(dst, o.str())) { msg = "cannot write " + dst.string(); return false; }
		msg = t->name + "." + v->id + ", " + std::to_string(rows) + " rows";
		return true;
	}

	bool PackTable(const Schema& schema, const fs::path& src, const fs::path& dst, std::string& msg)
	{
		std::ifstream in(src, std::ios::binary);
		if (!in) { msg = "cannot read"; return false; }
		std::string line;
		std::getline(in, line);
		if (!line.empty() && line.back() == '\r') line.pop_back();
		auto head = Split(line, '\t');
		if (head.empty() || head[0] != "#MuLegacy") { msg = "not a MuLegacy table text (first line must start with #MuLegacy)"; return false; }
		std::map<std::string, std::string> kv;
		for (size_t i = 1; i < head.size(); ++i)
		{
			size_t eq = head[i].find('=');
			if (eq != std::string::npos) kv[head[i].substr(0, eq)] = head[i].substr(eq + 1);
		}
		const Table* t = schema.TableByName(kv["table"]);
		if (!t) { msg = "unknown table " + kv["table"]; return false; }
		const Variant* v = nullptr;
		for (auto& x : t->variants) if (x.id == kv["variant"]) v = &x;
		if (!v) { msg = "unknown variant " + kv["variant"]; return false; }

		std::vector<std::vector<std::string>> rows;
		while (std::getline(in, line))
		{
			if (!line.empty() && line.back() == '\r') line.pop_back();
			if (line.empty() || line[0] == '#') continue;
			rows.push_back(Split(line, '\t'));
		}

		Bytes out;
		std::string err;
		if (v->layout == "text-indexed")
		{
			out.assign(v->magic.begin(), v->magic.end());
			out.resize(out.size() + 4);
			Put<uint32_t>(&out[v->magic.size()], (uint32_t)rows.size());
			for (auto& r : rows)
			{
				if (r.size() != 2) { msg = "text row needs 2 cells"; return false; }
				std::string s = UnescapeCell(r[1]);
				size_t p = out.size();
				out.resize(p + 8 + s.size());
				Put<uint32_t>(&out[p], (uint32_t)std::stoul(r[0]));
				Put<uint32_t>(&out[p + 4], (uint32_t)s.size());
				std::memcpy(&out[p + 8], s.data(), s.size());
				if (v->xorMode != "none") Xor3(&out[p + 8], s.size());
			}
		}
		else
		{
			size_t cells = 1;
			for (auto& f : v->fields) if (!f.pad) cells += f.count;
			Bytes area(rows.size() * v->rec, 0);
			for (size_t k = 0; k < rows.size(); ++k)
			{
				auto& r = rows[k];
				if (r.size() != cells) { msg = "row " + std::to_string(k) + ": expected " + std::to_string(cells) + " cells, got " + std::to_string(r.size()); return false; }
				uint8_t* p = &area[k * v->rec];
				size_t q = 0, c = 1;
				for (auto& f : v->fields)
					for (size_t i = 0; i < f.count; ++i, q += f.size)
						if (!f.pad && !ParseValue(f, r[c++], p + q, err)) { msg = "row " + std::to_string(k) + ": " + err; return false; }
				if (v->xorMode == "record") Xor3(p, v->rec);
			}
			if (v->xorMode == "stream") Xor3(area.data(), area.size());
			if (v->xorMode == "kayito") KayitoXor(area.data(), area.size());
			if (v->skip)
			{
				Bytes s;
				if (!ParseHex(kv["skip"], s) || s.size() != v->skip) { msg = "skip= in the first line must hold " + std::to_string(v->skip) + " bytes of hex"; return false; }
				XorPeriod(s.data(), s.size(), v->skipXor);
				out = s;
			}
			if (v->header == "counts32")
			{
				auto parts = Split(kv["counts"], ',');
				if (parts.size() != v->headerCounts) { msg = "counts= in the first line must list " + std::to_string(v->headerCounts) + " numbers"; return false; }
				Bytes c(4 * v->headerCounts);
				long long sum = 0;
				for (size_t i = 0; i < parts.size(); ++i) { int32_t x = (int32_t)std::stol(parts[i]); sum += x; Put<int32_t>(&c[i * 4], x); }
				if (sum != (long long)rows.size()) { msg = "counts= add up to " + std::to_string(sum) + " but there are " + std::to_string(rows.size()) + " rows"; return false; }
				Xor3(c.data(), c.size());
				out.insert(out.end(), c.begin(), c.end());
			}
			else if (v->header != "none")
			{
				size_t p = out.size();
				out.resize(p + 4);
				Put<uint32_t>(&out[p], (uint32_t)rows.size());
			}
			out.insert(out.end(), area.begin(), area.end());
			if (v->header == "count32cap")
			{
				Bytes tail;
				if (!kv["tail"].empty() && !ParseHex(kv["tail"], tail)) { msg = "bad tail= hex in the first line"; return false; }
				out.insert(out.end(), tail.begin(), tail.end());
			}
		}
		if (v->crcKey >= 0)
		{
			size_t s = CrcStart(*v);
			uint32_t c = CalculateCRC(out.data() + s, out.size() - s, (uint16_t)v->crcKey);
			Bytes keep;
			if (!v->crcCheck && ParseHex(kv["trailer"], keep) && keep.size() == 4) c = Get<uint32_t>(keep.data());
			size_t p = out.size();
			out.resize(p + 4);
			Put<uint32_t>(&out[p], c);
		}
		if (!v->sizes.empty() && std::find(v->sizes.begin(), v->sizes.end(), out.size()) == v->sizes.end())
			msg = "warning: " + std::to_string(out.size()) + " bytes, the client expects one of the variant's sizes; ";
		if (!WriteFile(dst, out)) { msg += "cannot write " + dst.string(); return false; }
		msg += t->name + "." + v->id + ", " + std::to_string(rows.size()) + " rows";
		return true;
	}
}
