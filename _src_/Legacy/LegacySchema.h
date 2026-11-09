// Schema-driven reader/writer for the old fixed-record data tables (Item.bmd, Skill.bmd, Gate.bmd, ...).
// Layouts live in a text file (LegacyFormats.txt) so they can be corrected without rebuilding.
#ifndef LEGACY_SCHEMA_H
#define LEGACY_SCHEMA_H

#include "LegacyCommon.h"

#include <map>

namespace Legacy
{
	enum class FieldType { U8, I8, U16, I16, U32, I32, F32, Str, Raw };

	struct Field
	{
		std::string name;
		FieldType type = FieldType::U8;
		size_t size = 1;    // bytes of one element
		size_t count = 1;   // array length
		bool pad = false;
	};

	struct Variant
	{
		std::string id;
		std::vector<size_t> sizes;   // exact file sizes; empty = any size that fits
		std::string layout = "fixed"; // fixed | text-indexed
		std::string header = "none";  // none | count32 | count32cap (count records + unused slots kept as is)
		                              // | counts32 (T xor'd i32 counts whose sum is the record count)
		size_t headerCounts = 0;      // T of counts32:T
		size_t skip = 0;              // bytes before the header/records, kept as is (shown decoded if skipXor)
		size_t skipXor = 0;           // FC CF AB restarting every skipXor bytes inside the skipped block (0 = plain)
		std::string xorMode = "record"; // record | stream | none | kayito (ServerInfo.bmd of kayito mains)
		int crcKey = -1;              // wkey of the trailing CRC, -1 = no CRC
		bool crcCheck = true;         // false: the file has a 4-byte trailer that is not a valid CRC (kept as is)
		std::string crcScope = "all"; // all: CRC over everything before it | body: records only (count header excluded)
		Bytes magic;                  // bytes before the count of a text-indexed file (e.g. "GT")
		long long probeRec = -1;      // probe=<record>:<offset>:<value> - byte of the decoded record that must match
		size_t probeOff = 0;          // (tells apart layouts that share size and CRC, e.g. 5- vs 6-class Item.bmd)
		int probeVal = 0;
		size_t rec = 0;
		std::string label;
		std::vector<Field> fields;
		size_t FieldBytes() const;
	};

	struct Table
	{
		std::string name;
		std::vector<std::string> globs;
		std::vector<Variant> variants;
	};

	class Schema
	{
	public:
		// file or directory of *.txt schema files
		bool Load(const fs::path& path, std::string& err);
		const Table* FindTable(const std::string& fileName) const;
		const Table* TableByName(const std::string& name) const;
		// first variant of the table whose size rule (and CRC) matches the data
		const Variant* Pick(const Table& t, const Bytes& data) const;
		size_t TableCount() const { return tables.size(); }

	private:
		bool LoadFile(const fs::path& path, std::string& err);
		std::vector<Table> tables;
	};

	// data file -> tab-separated text. Returns false (with message) if no variant fits.
	bool UnpackTable(const Schema& schema, const fs::path& src, const fs::path& dst, std::string& msg);
	// tab-separated text written by UnpackTable -> data file
	bool PackTable(const Schema& schema, const fs::path& src, const fs::path& dst, std::string& msg);
}

#endif // !LEGACY_SCHEMA_H
