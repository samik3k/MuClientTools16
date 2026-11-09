// MuLegacy: reads the data files of old MU Online clients (0.68 - 1.04, Season 0 - Season 4).
//
//   MuLegacy [-s <schema>] <file or folder> [output folder]   unpack everything it recognises
//   MuLegacy pack [-s <schema>] <table.txt> [output file]      rebuild a data table from its text
//
// The table layouts are read from LegacyFormats.txt (or a LegacyFormats folder) next to the exe.
#include "LegacyCommon.h"
#include "LegacyGraphic.h"
#include "LegacySchema.h"
#include "LegacyTerrain.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>

using namespace Legacy;

static fs::path g_exeDir;

static fs::path DefaultSchema()
{
	for (const char* n : { "LegacyFormats", "LegacyFormats.txt" })
	{
		fs::path p = g_exeDir / n;
		if (fs::exists(p)) return p;
	}
	return g_exeDir / "LegacyFormats.txt";
}

static void Usage()
{
	std::cout <<
		"MuLegacy - reader for MU Online client data 0.68 - 1.04 (Season 0 - 4)\n\n"
		"  MuLegacy [-s <schema>] <file or folder> [output folder]\n"
		"      Unpacks every recognised file (data tables -> .txt, OZ* images -> jpg/tga/bmp/...,\n"
		"      BMD models -> structure .txt (+ decrypted .bmd), terrain .att/.map/.obj -> decrypted\n"
		"      file + .txt, Enc1/Dec2.dat -> key .txt). Default output: <input>_out\n\n"
		"  MuLegacy pack [-s <schema>] <table.txt> [output file]\n"
		"      Rebuilds a data table from a .txt written by the unpacker.\n\n"
		"  -s  schema file or folder (default: LegacyFormats[.txt] next to the exe)\n";
}

struct Stats
{
	std::map<std::string, int> ok, skipped, failed, ignored;
};

enum class Kind { Table, Image, Model, Terrain, Key, Other };

static Kind Classify(const Schema& schema, const fs::path& p, Bytes* head)
{
	std::string ext = Lower(p.extension().string()), name = Lower(p.filename().string());
	if (IsImageExt(ext)) return Kind::Image;
	if (IsTerrainExt(ext)) return Kind::Terrain;
	if (IsKeyFile(name)) return Kind::Key;
	if (ext == ".bmd")
	{
		if (head && IsModelFile(*head)) return Kind::Model;
		if (schema.FindTable(p.filename().string())) return Kind::Table;
		return Kind::Model;
	}
	if (schema.FindTable(p.filename().string())) return Kind::Table;
	return Kind::Other;
}

static bool UnpackOne(const Schema& schema, const fs::path& src, const fs::path& dstNoExt, std::string& kindName, std::string& msg)
{
	Bytes head;
	{
		Bytes all;
		if (ReadFile(src, all)) head.assign(all.begin(), all.begin() + std::min<size_t>(all.size(), 16));
	}
	switch (Classify(schema, src, &head))
	{
	case Kind::Image: kindName = "image"; return UnpackImage(src, dstNoExt, msg);
	case Kind::Model: kindName = "model"; return UnpackModel(src, dstNoExt, msg);
	case Kind::Terrain: kindName = "terrain"; return UnpackTerrain(src, dstNoExt, msg);
	case Kind::Key: kindName = "keys"; return UnpackKeyFile(src, dstNoExt, msg);
	case Kind::Table:
	{
		kindName = "table";
		fs::path dst = dstNoExt;
		dst += ".txt";
		return UnpackTable(schema, src, dst, msg);
	}
	default: kindName = ""; return false;
	}
}

static int RunUnpack(const Schema& schema, const fs::path& in, fs::path out)
{
	std::error_code ec;
	bool isDir = fs::is_directory(in, ec);
	if (out.empty()) out = isDir ? fs::path(in.string() + "_out") : in.parent_path();
	std::vector<fs::path> files;
	if (isDir)
	{
		for (auto it = fs::recursive_directory_iterator(in, fs::directory_options::skip_permission_denied, ec);
			it != fs::recursive_directory_iterator(); it.increment(ec))
			if (it->is_regular_file(ec)) files.push_back(it->path());
		std::sort(files.begin(), files.end());
	}
	else files.push_back(in);

	Stats st;
	std::ostringstream log;
	for (auto& f : files)
	{
		fs::path rel = isDir ? fs::relative(f, in, ec) : f.filename();
		fs::path dstNoExt = out / rel;
		dstNoExt.replace_extension();
		std::string kind, msg;
		bool ok = UnpackOne(schema, f, dstNoExt, kind, msg);
		std::string ext = Lower(f.extension().string());
		if (kind.empty()) { st.ignored[ext.empty() ? "(none)" : ext]++; continue; }
		std::string tag = ok ? "ok  " : (msg.find("skipped") != std::string::npos ? "skip" : "FAIL");
		(ok ? st.ok : tag == "skip" ? st.skipped : st.failed)[kind]++;
		log << "[" << tag << "] " << rel.generic_string() << " : " << msg << "\n";
		if (!ok && tag == "FAIL") std::cout << "[FAIL] " << rel.generic_string() << " : " << msg << "\n";
	}

	std::ostringstream sum;
	sum << "\nInput : " << in.string() << "\nOutput: " << out.string() << "\n\n";
	sum << "Kind       ok   skipped  failed\n";
	for (const char* k : { "table", "image", "model", "terrain", "keys" })
		sum << "  " << k << std::string(9 - strlen(k), ' ') << st.ok[k] << "\t" << st.skipped[k] << "\t" << st.failed[k] << "\n";
	if (!st.ignored.empty())
	{
		sum << "Not data files (left alone):";
		for (auto& [e, n] : st.ignored) sum << " " << e << " " << n;
		sum << "\n";
	}
	std::cout << sum.str();
	if (isDir) WriteText(out / "MuLegacy.log", log.str() + sum.str());
	else std::cout << log.str();
	int failed = 0;
	for (auto& [k, n] : st.failed) failed += n;
	return failed ? 2 : 0;
}

int main(int argc, char** argv)
{
	g_exeDir = fs::absolute(fs::path(argv[0])).parent_path();
	std::vector<std::string> a(argv + 1, argv + argc);
	fs::path schemaPath;
	for (size_t i = 0; i < a.size(); ++i)
		if (a[i] == "-s" && i + 1 < a.size()) { schemaPath = a[i + 1]; a.erase(a.begin() + i, a.begin() + i + 2); break; }
	if (a.empty() || a[0] == "-h" || a[0] == "--help") { Usage(); return a.empty() ? 1 : 0; }
	if (schemaPath.empty()) schemaPath = DefaultSchema();

	Schema schema;
	std::string err;
	if (!schema.Load(schemaPath, err))
	{
		std::cout << "Schema error: " << err << "\n";
		return 1;
	}

	if (a[0] == "pack")
	{
		if (a.size() < 2) { Usage(); return 1; }
		fs::path src = a[1], dst = a.size() > 2 ? fs::path(a[2]) : fs::path();
		if (dst.empty())
		{
			// default: <name>.<original extension> taken from the "source=" entry of the text header
			Bytes t;
			ReadFile(src, t);
			std::string first(t.begin(), std::find(t.begin(), t.end(), '\n'));
			size_t s = first.find("source=");
			std::string orig = s == std::string::npos ? "" : first.substr(s + 7, first.find('\t', s) - s - 7);
			dst = src.parent_path() / (orig.empty() ? src.stem().string() + ".bmd" : orig);
			if (fs::exists(dst)) dst.replace_filename(dst.stem().string() + ".new" + dst.extension().string());
		}
		std::string msg;
		bool ok = PackTable(schema, src, dst, msg);
		std::cout << (ok ? "[ok]   " : "[FAIL] ") << dst.string() << " : " << msg << "\n";
		return ok ? 0 : 2;
	}
	return RunUnpack(schema, a[0], a.size() > 1 ? fs::path(a[1]) : fs::path());
}
