// Tables that only the current Global MU (GMO) client has.
// Layouts were found from the data (record boundaries from the XOR phase, CRC keys by trying all 65536 wkeys);
// field names are from the values unless the meaning is clear - unknown ones are Unk<offset>.
// Text is cp949 (Korean) in the Korean/base files.
#ifndef GMO_BMD_H
#define GMO_BMD_H

#include <sstream>

//------------------------------------------------------------------------
//--Structures
//------------------------------------------------------------------------

// ArcaBattleBootyMix.bmd: [count][records][CRC E2F1] - 10 rows: 1..10, 11..20 ... 91..100, rate 55/50/45, 5
STRUCT(GMO_ARCA_BATTLE_BOOTY_MIX) LAZY
(
	int Index;
	int MinCount;
	int MaxCount;
	int Rate;
	int Unk16;
);

// FcsData.bmd: [count][records][CRC DA42] - login launcher config (wzLogin / kbLogin, SVR002, login URLs)
STRUCT(GMO_FCS_DATA) LAZY
(
	BYTE Index;
	char Name[260];
	char ServerCode[260];
	char LoginUrl[260];
	char AlphaLoginUrl[260];
	char Language[260];
	char Unk1301[260];
);

// WebAPIMessage.bmd: [count][records][CRC A35B] - login / web API errors.
// Webzen's own source is shipped next to it (webapimessage.bmd.txt): Index Code "Message" Unk268 Unk272
STRUCT(GMO_WEB_API_MESSAGE) LAZY
(
	int Index;
	int Code;
	char Message[260];
	int Unk268;
	int Unk272;
);

// attributevariation.bmd: [count][records][CRC E2F1] - 6 elemental variants per monster (folder + texture)
STRUCT(GMO_ATTRIBUTE_VARIATION) LAZY
(
	int Type;
	int Monster;
	int Unk8;
	int Unk12;
	int Unk16;
	int Attribute[6];
	char Path1[260];
	char Path2[260];
	char Path3[260];
	char Path4[260];
	char Path5[260];
	char Path6[260];
	char Texture1[256];
	char Texture2[256];
	char Texture3[256];
	char Texture4[256];
	char Texture5[256];
	char Texture6[256];
);

// ItemLevelTooltip.bmd: 128 slots, no counter, [CRC E2F1 over everything]. After the name, 11 tooltip lines of
// (Text.bmd index?, value); unused lines are 65535, 0.
STRUCT(GMO_ITEM_LEVEL_TOOLTIP) LAZY
(
	WORD Index;
	char Name[64];
	BYTE Unk66;
	BYTE Unk67;
	WORD Unk68;
	WORD Line1Text;
	WORD Line1Value;
	WORD Line2Text;
	WORD Line2Value;
	WORD Line3Text;
	WORD Line3Value;
	WORD Line4Text;
	WORD Line4Value;
	WORD Line5Text;
	WORD Line5Value;
	WORD Line6Text;
	WORD Line6Value;
	WORD Line7Text;
	WORD Line7Value;
	WORD Line8Text;
	WORD Line8Value;
	WORD Line9Text;
	WORD Line9Value;
	WORD Line10Text;
	WORD Line10Value;
	WORD Line11Text;
	WORD Line11Value;
);

// Gate.bmd (GMO): 512 gates of 14 bytes, no counter, no CRC. The 12-byte gate of older clients + MaxLevel
// (400 on every used gate). Flag: 0 = spawn area, 1 = entrance, 2 = exit.
STRUCT(GMO_GATE) LAZY
(
	BYTE Flag;
	BYTE Map;
	BYTE X1;
	BYTE Y1;
	BYTE X2;
	BYTE Y2;
	WORD Target;
	WORD Dir;
	WORD Level;
	WORD MaxLevel;
);

// JewelOfHarmonySmelt.bmd: 8192 slots (one per item, ItemId = group*512+index), whole file XOR'd as one
// stream, no counter, no CRC. Items that can be smelted.
STRUCT(GMO_JEWEL_OF_HARMONY_SMELT) LAZY
(
	DWORD ItemId;
	char Name[60];
	DWORD Unk64;
);

// Minimap.bmd: [count] then per map: [map number][u8 entry count][entries], every part XOR'd as its own
// block, [CRC CE54 over everything]. Kind 1 = NPC, 2 = entrance.
STRUCT(GMO_MINIMAP_ENTRY) LAZY
(
	int Kind;
	int X;
	int Y;
	int Unk12;
	int Unk16;
	char Name[100];
);

static_assert(sizeof(GMO_ARCA_BATTLE_BOOTY_MIX) == 20, "ArcaBattleBootyMix.bmd record");
static_assert(sizeof(GMO_FCS_DATA) == 1561, "FcsData.bmd record");
static_assert(sizeof(GMO_WEB_API_MESSAGE) == 276, "WebAPIMessage.bmd record");
static_assert(sizeof(GMO_ATTRIBUTE_VARIATION) == 3140, "attributevariation.bmd record");
static_assert(sizeof(GMO_ITEM_LEVEL_TOOLTIP) == 114, "ItemLevelTooltip.bmd record");
static_assert(sizeof(GMO_GATE) == 14, "Gate.bmd (GMO) record");
static_assert(sizeof(GMO_JEWEL_OF_HARMONY_SMELT) == 68, "JewelOfHarmonySmelt.bmd record");
static_assert(sizeof(GMO_MINIMAP_ENTRY) == 120, "Minimap.bmd entry");

//------------------------------------------------------------------------
//--Helpers
//------------------------------------------------------------------------

// "//Field1\tField2..." of T with an extra first column
template<typename T>
inline std::string GmoHeader(const char* firstColumn)
{
	std::string label = T::GetLabel();
	while (!label.empty() && (label.back() == '\n' || label.back() == '\r')) label.pop_back();
	size_t a = 0;
	while (a < label.size() && (label[a] == '/' || label[a] == '\t')) a++;
	return std::string("//") + firstColumn + "\t" + label.substr(a);
}

// reads the cells of 'line' from position 'a' on into *ptr, using T's field list (same rules as TxtConvertor)
template<typename T>
inline void GmoParseCells(std::string line, size_t a, T* ptr)
{
	static const std::vector<OffsetInfo> OFFSET = T::GetOffset();
	if (line.empty() || line.back() != '\t') line += '\t';
	size_t b = line.find('\t', a);
	size_t i = 0;
	while (b != std::string::npos && i < OFFSET.size())
	{
		if (b > a)
		{
			std::string s = line.substr(a, b - a);
			if (OFFSET[i].Type == LAZY_TYPE_FLAG::_CSTR_)
			{
				size_t len = (i < OFFSET.size() - 1 ? OFFSET[i + 1].Offset : sizeof(T)) - OFFSET[i].Offset;
				if (len - 1 < s.length()) s.resize(len - 1);
			}
			sscanf(s.c_str(), OFFSET[i].Format.c_str(), (void*)((size_t)ptr + OFFSET[i].Offset));
		}
		i++;
		a = b + 1;
		b = line.find('\t', a);
	}
}

inline bool GmoIsComment(const std::string& line)
{
	return line.size() >= 2 && line[0] == '/' && line[1] == '/';
}

//------------------------------------------------------------------------
//--Tables with a counter: [count][records, XOR per record][CRC over the records]
//------------------------------------------------------------------------

template<typename T, WORD WKEY>
class GmoCountedBmd : public TxtConvertor<T>
{
public:
	GmoCountedBmd() : TxtConvertor<T>(WKEY) {};
	virtual ~GmoCountedBmd() {};
};

typedef GmoCountedBmd<GMO_ARCA_BATTLE_BOOTY_MIX, 0xE2F1> ArcaBattleBootyMixBmd;
typedef GmoCountedBmd<GMO_FCS_DATA, 0xDA42> FcsDataBmd;
typedef GmoCountedBmd<GMO_WEB_API_MESSAGE, 0xA35B> WebAPIMessageBmd;
typedef GmoCountedBmd<GMO_ATTRIBUTE_VARIATION, 0xE2F1> AttributeVariationBmd;

//------------------------------------------------------------------------
//--Fixed-size tables: N slots, no counter. The txt has an Index column (slot number); empty (all-zero)
//--slots are left out and come back as zeros.
//------------------------------------------------------------------------

template<typename T, size_t N>
class GmoSlotsBmd : public TxtConvertor<T>
{
public:
	static const size_t COUNT = N;

	// wkey < 0: no CRC; stream: one XOR stream over all slots instead of one per slot
	GmoSlotsBmd(int wkey, bool stream)
		: TxtConvertor<T>(wkey < 0 ? 0 : (WORD)wkey), _crc(wkey >= 0), _stream(stream)
	{
		this->hasCounter = false;
		this->hasCRC = _crc;
	};
	virtual ~GmoSlotsBmd() {};

	size_t FileSize() const { return COUNT * sizeof(T) + (_crc ? 4 : 0); }

protected:
	virtual void CommentIn(const std::string& line) {};	// comment lines of the txt (for subclasses)
	virtual void RecordIn(T* ptr) {};						// called after a row was read

	BOOL Decrypt()
	{
		auto& buf = this->_buf;
		const size_t size = sizeof(T);
		if (buf.size() != FileSize())
		{
			std::cout << "[ERROR] File size " << buf.size() << ", expected " << FileSize() << std::endl;
			return FALSE;
		}
		XorSlots();
		this->_map.clear();
		for (size_t i = 0; i < COUNT; i++)
		{
			T* ptr = (T*)&buf[i * size];
			const BYTE* p = (const BYTE*)ptr;
			if (std::any_of(p, p + size, [](BYTE c) { return c != 0; }))
				this->InsertToMap((int)i, ptr);
		}
		return TRUE;
	}

	BOOL Encrypt()
	{
		auto& buf = this->_buf;
		assert(buf.size() == FileSize());
		XorSlots();
		if (_crc)
			*(DWORD*)&buf[buf.size() - 4] = this->CalculateCRC(&buf[0], COUNT * sizeof(T), this->_wkey);
		return TRUE;
	}

	void TxtOut(std::ofstream& os)
	{
		assert(os);
		this->MakeLabelEx(os);
		os << GmoHeader<T>("Index") << std::endl;
		for (auto it = this->_map.begin(); it != this->_map.end(); it++)
		{
			os << it->first << '\t';
			this->OffsetOut(os, it->second);
			os << std::endl;
		}
	}

	void TxtIn(std::ifstream& is)
	{
		assert(is);
		auto& buf = this->_buf;
		buf.assign(FileSize(), 0);
		std::string line;
		while (getline(is, line))
		{
			if (!line.empty() && line.back() == '\r') line.pop_back();
			if (GmoIsComment(line)) { CommentIn(line); continue; }
			size_t b = line.find('\t');
			if (line.empty() || b == 0) continue;
			int id = -1;
			sscanf(line.substr(0, b).c_str(), "%d", &id);
			if (id < 0 || (size_t)id >= COUNT)
			{
				std::cout << "[WARNING] Index " << id << " is outside 0.." << COUNT - 1 << ", row skipped" << std::endl;
				continue;
			}
			T* ptr = (T*)&buf[id * sizeof(T)];
			memset(ptr, 0, sizeof(T));
			if (b != std::string::npos)
				GmoParseCells(line, b + 1, ptr);
			RecordIn(ptr);
		}
	}

private:
	void XorSlots()
	{
		auto& buf = this->_buf;
		const size_t size = sizeof(T);
		if (_stream)
			this->Xor3Byte(&buf[0], COUNT * size);
		else
			for (size_t i = 0; i < COUNT; i++)
				this->Xor3Byte(&buf[i * size], size);
	}

	bool _crc;
	bool _stream;
};

class GateGmoBmd : public GmoSlotsBmd<GMO_GATE, 512>
{
public:
	GateGmoBmd() : GmoSlotsBmd(-1, false) {};
};

class ItemLevelTooltipBmd : public GmoSlotsBmd<GMO_ITEM_LEVEL_TOOLTIP, 128>
{
public:
	ItemLevelTooltipBmd() : GmoSlotsBmd(0xE2F1, false) {};
};

// Names are padded after the terminator: with 0xFF in the Korean / English files, with zeros in others.
// The txt keeps which one in a "//NameFill=FF" line so packing gives back the same file.
class JewelOfHarmonySmeltBmd : public GmoSlotsBmd<GMO_JEWEL_OF_HARMONY_SMELT, 8192>
{
public:
	JewelOfHarmonySmeltBmd() : GmoSlotsBmd(-1, true), _fillFF(false) {};

private:
	void MakeLabelEx(std::ofstream& os)
	{
		bool ff = false;
		for (auto it = _map.begin(); it != _map.end(); it++)
		{
			const char* name = it->second->Name;
			size_t n = strnlen(name, sizeof(it->second->Name));
			if (n + 1 < sizeof(it->second->Name)) { ff = (BYTE)name[n + 1] == 0xFF; break; }
		}
		os << "//NameFill=" << (ff ? "FF" : "00") << "\t(bytes after the name: FF or 00 - keep this line)" << std::endl;
	}

	void CommentIn(const std::string& line)
	{
		if (line.compare(0, 13, "//NameFill=FF") == 0) _fillFF = true;
		if (line.compare(0, 13, "//NameFill=00") == 0) _fillFF = false;
	}

	void RecordIn(GMO_JEWEL_OF_HARMONY_SMELT* ptr)
	{
		if (!_fillFF) return;
		size_t n = strnlen(ptr->Name, sizeof(ptr->Name));
		for (size_t i = n + 1; i < sizeof(ptr->Name); i++)
			ptr->Name[i] = (char)0xFF;
	}

	bool _fillFF;
};

//------------------------------------------------------------------------
//--Minimap.bmd: grouped by map. The txt has a Map column; rows of the same map are written together
//--(in the order the maps first appear).
//------------------------------------------------------------------------

class MinimapBmd : public TxtConvertor<GMO_MINIMAP_ENTRY>
{
public:
	MinimapBmd() : TxtConvertor<GMO_MINIMAP_ENTRY>(0xCE54) {};
	virtual ~MinimapBmd() {};

private:
	typedef GMO_MINIMAP_ENTRY T;

	static DWORD XorDword(const BYTE* p, MinimapBmd* self)
	{
		BYTE b[4] = { p[0], p[1], p[2], p[3] };
		self->Xor3Byte(b, 4);
		return *(DWORD*)b;
	}

	BOOL Decrypt()
	{
		auto& buf = _buf;
		_rows.clear();
		if (buf.size() < 8) return FALSE;
		const size_t end = buf.size() - 4;	// CRC
		DWORD groups = XorDword(&buf[0], this);
		size_t p = 4;
		for (DWORD g = 0; g < groups; g++)
		{
			if (p + 5 > end) { std::cout << "[ERROR] Minimap: map header past the end" << std::endl; return FALSE; }
			DWORD map = XorDword(&buf[p], this);
			BYTE count = buf[p + 4];
			Xor3Byte(&count, 1);
			p += 5;
			if (p + count * sizeof(T) > end) { std::cout << "[ERROR] Minimap: entries past the end" << std::endl; return FALSE; }
			Xor3Byte(&buf[p], count * sizeof(T));
			for (BYTE i = 0; i < count; i++)
				_rows.push_back(std::make_pair(map, (T*)&buf[p + i * sizeof(T)]));
			p += count * sizeof(T);
		}
		if (p != end) std::cout << "[WARNING] Minimap: " << end - p << " bytes after the last map" << std::endl;
		// TxtConvertor::ComposeTxt wants a non-empty map
		_map.clear();
		if (!_rows.empty()) _map.insert(std::make_pair(0, _rows[0].second));
		return TRUE;
	}

	void TxtOut(std::ofstream& os)
	{
		assert(os);
		os << GmoHeader<T>("Map") << std::endl;
		for (auto& r : _rows)
		{
			os << r.first << '\t';
			OffsetOut(os, r.second);
			os << std::endl;
		}
	}

	void TxtIn(std::ifstream& is)
	{
		assert(is);
		std::vector<std::pair<DWORD, T>> rows;
		std::vector<DWORD> order;
		std::string line;
		while (getline(is, line))
		{
			if (!line.empty() && line.back() == '\r') line.pop_back();
			if (GmoIsComment(line)) continue;
			size_t b = line.find('\t');
			if (line.empty() || b == 0 || b == std::string::npos) continue;
			DWORD map = (DWORD)strtoul(line.substr(0, b).c_str(), nullptr, 10);
			T rec;
			memset(&rec, 0, sizeof(T));
			GmoParseCells(line, b + 1, &rec);
			rows.push_back(std::make_pair(map, rec));
			if (std::find(order.begin(), order.end(), map) == order.end()) order.push_back(map);
		}

		auto& buf = _buf;
		buf.assign(4, 0);
		*(DWORD*)&buf[0] = (DWORD)order.size();
		Xor3Byte(&buf[0], 4);
		for (DWORD map : order)
		{
			std::vector<BYTE> area;
			for (auto& r : rows)
				if (r.first == map)
					area.insert(area.end(), (BYTE*)&r.second, (BYTE*)&r.second + sizeof(T));
			size_t count = area.size() / sizeof(T);
			if (count > 255)
			{
				std::cout << "[ERROR] Map " << map << " has " << count << " rows, the file allows 255" << std::endl;
				count = 255;
				area.resize(255 * sizeof(T));
			}
			BYTE head[5];
			*(DWORD*)head = map;
			Xor3Byte(head, 4);
			head[4] = (BYTE)count;
			Xor3Byte(&head[4], 1);
			buf.insert(buf.end(), head, head + 5);
			if (!area.empty())
			{
				Xor3Byte(area.data(), area.size());
				buf.insert(buf.end(), area.begin(), area.end());
			}
		}
		buf.resize(buf.size() + 4);	// CRC
	}

	BOOL Encrypt()
	{
		auto& buf = _buf;
		*(DWORD*)&buf[buf.size() - 4] = CalculateCRC(&buf[0], buf.size() - 4, _wkey);
		return TRUE;
	}

	std::vector<std::pair<DWORD, T*>> _rows;
};

#endif
