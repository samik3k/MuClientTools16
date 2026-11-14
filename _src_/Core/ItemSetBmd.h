
#ifndef ITEM_SET_TYPE_BMD_H
#define ITEM_SET_TYPE_BMD_H

#define _ITEM_SET_TYPE_BMD_WKEY_ 0xE5F1

// itemsettype.bmd: one record per item (21 x 512), no counter, CRC at the end.
// Season 16 = ITEM_SET_TYPE (6 bytes), GMO = ITEM_SET_TYPE_GMO (10 bytes, WORD tiers).
// ItemSetType_bmd.exe picks the version by itself (file size / txt header).
template<typename T>
class ItemSetTypeBmdT : public TxtConvertor<T>
{
public:
	static const size_t COUNT = ITEM_CATEGORY_MAX * ITEM_INDEX_MAX;
	static const size_t FILE_SIZE = COUNT * sizeof(T) + 4;
	static const char* Header();	// txt header line

	ItemSetTypeBmdT() : TxtConvertor<T>(_ITEM_SET_TYPE_BMD_WKEY_) {};
	virtual ~ItemSetTypeBmdT() {};

private:
	BOOL Decrypt()
	{
		auto& buf = this->_buf;
		assert(buf.size());
		const size_t size = sizeof(T);
		size_t pos = 0;
		this->_map.clear();
		for (size_t i = 0; i < COUNT; i++)
		{
			if (pos + size > buf.size()) return TRUE; //ignore error
			this->Xor3Byte(&buf[pos], size);
			T* ptr = (T*)&buf[pos];
			if (ptr->Tier1 | ptr->Tier2 | ptr->Tier3 | ptr->Tier4 | ptr->Unk1 | ptr->Unk2)
				this->InsertToMap((int)i, ptr);
			pos += size;
		}
		return TRUE;
	}

	BOOL Encrypt()
	{
		auto& buf = this->_buf;
		const size_t size = sizeof(T);
		assert(buf.size() == FILE_SIZE);
		for (size_t i = 0; i < COUNT; i++)
			this->Xor3Byte(&buf[i * size], size);
		*(DWORD*)&buf[buf.size() - 4] = this->CalculateCRC(&buf[0], COUNT * size, this->_wkey);
		return TRUE;
	}

	void TxtOut(std::ofstream& os)
	{
		assert(os);
		os << Header() << std::endl;
		for (auto it = this->_map.begin(); it != this->_map.end(); it++)
		{
			T* ptr = it->second;
			int item_cat = it->first / 512;
			int item_idx = it->first % 512;
			if (item_cat < ITEM_CATEGORY_MAX && item_idx < ITEM_INDEX_MAX)
			{
				os << item_cat << '\t' << item_idx << '\t'
					<< (int)ptr->Tier1 << '\t' << (int)ptr->Tier2 << '\t'
					<< (int)ptr->Tier3 << '\t' << (int)ptr->Tier4 << '\t'
					<< (int)ptr->Unk1 << '\t' << (int)ptr->Unk2 << '\t' << std::endl;
			}
		}
	}

	void TxtIn(std::ifstream& is)
	{
		assert(is);
		auto& buf = this->_buf;
		const size_t size = sizeof(T);
		buf.resize(FILE_SIZE);
		memset(buf.data(), 0x0, buf.size());

		std::string line;
		while (getline(is, line))
		{
			if (line.size() >= 2 && line[0] == '/' && line[1] == '/')
				continue;
			int v[8] = {};
			if (sscanf(line.c_str(), "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d",
				&v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7]) < 2)
				continue;
			if (v[0] < 0 || v[0] >= ITEM_CATEGORY_MAX || v[1] < 0 || v[1] >= ITEM_INDEX_MAX)
				continue;
			T* ptr = (T*)&buf[ITEM_MAKE(v[0], v[1]) * size];
			ptr->Tier1 = v[2]; ptr->Tier2 = v[3]; ptr->Tier3 = v[4]; ptr->Tier4 = v[5];
			ptr->Unk1 = v[6]; ptr->Unk2 = v[7];
		}
	}
};

template<> inline const char* ItemSetTypeBmdT<ITEM_SET_TYPE>::Header()
{ return "//ItemCat\tItemIndex\tTier1\tTier2\tTier3\tTier4\tUnk1\tUnk2"; }
template<> inline const char* ItemSetTypeBmdT<ITEM_SET_TYPE_GMO>::Header()
{ return "//ItemCat\tItemIndex\tTier1\tTier2\tTier3\tTier4\tUnk1\tUnk2\t//GMO"; }

typedef ItemSetTypeBmdT<ITEM_SET_TYPE> ItemSetTypeBmd;
typedef ItemSetTypeBmdT<ITEM_SET_TYPE_GMO> ItemSetTypeBmdGMO;

#endif

//=========================================================

#ifndef ITEM_SET_OPTION_BMD_H
#define ITEM_SET_OPTION_BMD_H

#define _ITEM_SET_OPTION_BMD_WKEY_ 0xA2F1

// itemsetoption.bmd: fixed number of sets, no counter, CRC at the end.
// Season 16 = 255 x ITEM_SET_OPTION (360 bytes), GMO = 300 x ITEM_SET_OPTION_GMO (372 bytes).
template<typename T, size_t N>
class ItemSetOptionBmdT : public TxtConvertor<T>
{
public:
	static const size_t COUNT = N;
	static const size_t FILE_SIZE = COUNT * sizeof(T) + 4;
	static const std::string& Header()	// txt header line (for version detection)
	{
		static const std::string LABEL = [] {
			std::string s = "//ID\t" + T::GetLabel();
			while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
			return s;
		}();
		return LABEL;
	}

	ItemSetOptionBmdT() : TxtConvertor<T>(_ITEM_SET_OPTION_BMD_WKEY_) {};
	virtual ~ItemSetOptionBmdT() {};

private:
	BOOL Decrypt()
	{
		auto& buf = this->_buf;
		assert(buf.size());
		const size_t size = sizeof(T);
		this->_map.clear();
		for (size_t i = 0, pos = 0; i < COUNT; i++, pos += size)
		{
			if (pos + size > buf.size()) return TRUE; //ignore error
			this->Xor3Byte(&buf[pos], size);
			this->InsertToMap((int)i, (T*)&buf[pos]);
		}
		return TRUE;
	}

	BOOL Encrypt()
	{
		auto& buf = this->_buf;
		const size_t size = sizeof(T);
		assert(buf.size() == FILE_SIZE);
		for (size_t i = 0; i < COUNT; i++)
			this->Xor3Byte(&buf[i * size], size);
		*(DWORD*)&buf[buf.size() - 4] = this->CalculateCRC(&buf[0], COUNT * size, this->_wkey);
		return TRUE;
	}

	void TxtOut(std::ofstream& os)
	{
		assert(os);
		static const std::string LABEL = "//ID\t" + T::GetLabel();
		os << LABEL << std::endl;
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
		static const std::vector<OffsetInfo> OFFSET = T::GetOffset();
		auto& buf = this->_buf;
		const size_t size = sizeof(T);
		buf.resize(FILE_SIZE);

		std::string line;
		while (getline(is, line))
		{
			if (line.size() >= 2 && line[0] == '/' && line[1] == '/')
				continue;

			line += '\t';
			size_t a = 0;
			size_t b = line.find('\t', a);
			if (b <= a)
				continue;	//1st column (ID) must not be empty
			int id = -1;
			sscanf(line.substr(a, b - a).c_str(), "%d", &id);
			if (id < 0 || (size_t)id >= COUNT)
				continue;

			T* ptr = (T*)&buf[id * size];
			a = b + 1;
			b = line.find('\t', a);
			size_t i = 0;
			do
			{
				if (b > a)
				{
					std::string s = line.substr(a, b - a);
					size_t pos = OFFSET[i].Offset + (size_t)ptr;
					sscanf(s.c_str(), OFFSET[i].Format.c_str(), (void*)pos);
				}
				i++;
				a = b + 1;
				b = line.find('\t', a);
			} while (b != std::string::npos && i < OFFSET.size());
		}
	}
};

typedef ItemSetOptionBmdT<ITEM_SET_OPTION, 255> ItemSetOptionBmd;
typedef ItemSetOptionBmdT<ITEM_SET_OPTION_GMO, 300> ItemSetOptionBmdGMO;

#endif

//=========================================================

#ifndef ITEM_SET_OPTION_TEXT_BMD_H
#define ITEM_SET_OPTION_TEXT_BMD_H

#define _ITEM_SET_OPTION_TEXT_BMD_WKEY_ _COMMON_TXTCONVERTOR_WKEY_

typedef TxtConvertor<ITEM_SET_OPTION_TEXT> ItemSetOptionTextBmdConvert;
class ItemSetOptionTextBmd : public ItemSetOptionTextBmdConvert
{
public:
	ItemSetOptionTextBmd() : ItemSetOptionTextBmdConvert(_ITEM_SET_OPTION_TEXT_BMD_WKEY_) {};
	virtual ~ItemSetOptionTextBmd() { };
};

#endif