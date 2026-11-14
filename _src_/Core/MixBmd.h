
#ifndef MIX_BMD_H
#define MIX_BMD_H

// mix.bmd: [group counts][records of MIX_INFO (660 bytes)], XOR3, no CRC.
// Season 16 = 13 groups, GMO = 14 groups.
// Mix_bmd.exe picks the version by itself (header counts vs file size / "//GMO mix.bmd" txt line).
template<int NGROUP>
class MixBmdT : public TxtConvertor<MIX_INFO>
{
public:
	MixBmdT() : TxtConvertor<MIX_INFO>() { hasCounter = false; hasCRC = false; };
	virtual ~MixBmdT() { };

	static const char* Header();	// first txt line ("" = none, Season 16 as before)

	// Does this .bmd have NGROUP group counts that add up to its size?
	bool MatchBmd(const char* path)
	{
		std::ifstream is(path, std::ios::in | std::ios::binary);
		std::vector<BYTE> head(NGROUP * 4);
		if (!is.read((char*)head.data(), head.size()))
			return false;
		Xor3Byte(head.data(), head.size());
		unsigned long long total = head.size();
		for (int i = 0; i < NGROUP; i++)
			total += (unsigned long long)((DWORD*)head.data())[i] * sizeof(MIX_INFO);
		return total == (unsigned long long)fs::file_size(path);
	}

private:
	BOOL Decrypt();
	BOOL Encrypt();
	void TxtOut(std::ofstream & os);
	void TxtIn(std::ifstream& is);
};

template<int NGROUP>
BOOL MixBmdT<NGROUP>::Decrypt()
{
	size_t group = NGROUP;
	size_t size = sizeof(MIX_INFO);
	size_t pos = 0;

	this->_map.clear();

	this->Xor3Byte(&this->_buf[0], group * 4);
	size_t* counts = (size_t*)&this->_buf[0];
	pos += group * 4;
	for (size_t i = 0; i < group; i++)
	{
		size_t count = counts[i];
		if (count > 10000)
		{
			std::cout << "Error: MixBmd::Decrypt() count > 10000." << std::endl;
			return FALSE;
		}

		for (size_t j = 0; j < count; j++)
		{
			if (pos + size > this->_buf.size())
			{
				std::cout << "Error: MixBmd::Decrypt() pos + size > _buf.size()." << std::endl;
				return FALSE;
			}

			this->Xor3Byte(&this->_buf[pos], size);

			MIX_INFO *ptr = (MIX_INFO *)&this->_buf[pos];
			int key = (i << 16) | j;
			this->InsertToMap(key, ptr);

			pos += size;
		}
	}

	return TRUE;
}

template<int NGROUP>
BOOL MixBmdT<NGROUP>::Encrypt()
{
	assert(this->_buf.size() > 0);

	size_t group = NGROUP;
	size_t size = sizeof(MIX_INFO);
	size_t pos = 0;
	size_t counts[NGROUP];

	memcpy(counts, &this->_buf[0], NGROUP * 4);
	this->Xor3Byte(&this->_buf[0], NGROUP * 4);

	pos += NGROUP * 4;
	for (size_t i = 0; i < group; i++)
	{
		size_t count = counts[i];
		if (count > 10000)
		{
			std::cout << "Error: MixBmd::Encrypt() count > 10000." << std::endl;
			return FALSE;
		}

		for (size_t j = 0; j < count; j++)
		{
			if (pos + size > this->_buf.size())
			{
				std::cout << "Error: MixBmd::Encrypt() pos + size > _buf.size()." << std::endl;
				return FALSE;
			}

			this->Xor3Byte(&this->_buf[pos], size);
			pos += size;
		}
	}

	return TRUE;
}

template<int NGROUP>
void MixBmdT<NGROUP>::TxtOut(std::ofstream & os)
{
	assert(os);

	if (*Header())
		os << Header() << std::endl;
	os << "//Group\tID\tType\tDesc_Msg_1\tDesc_Msg_2\tDesc_Msg_3\tBad_Msg_1\tBad_Msg_2\tBad_Msg_3\t"
		"Good_Msg_1\tGood_Msg_2\tGood_Msg_3\tHeight\tWidth\tMinLevel\ttMaxSuccesRate\tMoneyRule\tMoneyReq\t" 
		"UnkByte_1\tUnkByte_2\tUnkByte_3\tUnkByte_4\tUnkByte_5\tUnkByte_6"<< std::endl;
	os << "//(Rules Count)" << std::endl;
	os << "//Rule\tDiv" << std::endl;
	os << "//(Items Count)" << std::endl;
	os << "//StartID\tEndID\tMinLvl\tMaxLvl\tMinOpt\tMaxOpt\tMinDur\tMaxDur\tMinCount\tMaxCount\tType" << std::endl;
	os << "//=================================================================================================" << std::endl;

	for (auto it = this->_map.begin(); it != this->_map.end(); it++)
	{
		MIX_INFO* ptr = it->second;
		int group = (it->first >> 16);
		os << group << '\t';
		os << ptr->ID << '\t';
		os << ptr->Type << '\t';
		os << ptr->Desc_Msg_1 << '\t';
		os << ptr->Desc_Msg_2 << '\t';
		os << ptr->Desc_Msg_3 << '\t';
		os << ptr->Bad_Msg_1 << '\t';
		os << ptr->Bad_Msg_2 << '\t';
		os << ptr->Bad_Msg_3 << '\t';
		os << ptr->Good_Msg_1 << '\t';
		os << ptr->Good_Msg_2 << '\t';
		os << ptr->Good_Msg_3 << '\t';
		os << ptr->Height << '\t';
		os << ptr->Width << '\t';
		os << ptr->MinLevel << '\t';
		os << ptr->MaxSuccesRate << '\t';
		os << (DWORD)ptr->MoneyRule << '\t';
		os << ptr->MoneyReq << '\t';
		os << (DWORD)ptr->Unk_1 << '\t';
		os << (DWORD)ptr->Unk_2 << '\t';
		os << (DWORD)ptr->Unk_3 << '\t';
		os << (DWORD)ptr->Unk_4 << '\t';
		os << (DWORD)ptr->Unk_5 << '\t';
		os << (DWORD)ptr->Unk_6 << '\t';

		os << std::endl;

		os << ptr->RulesCount << std::endl;
		for (int i = 0; i < ptr->RulesCount; i++)
		{
			MIX_RULE * rule = &ptr->Rules[i];
			os << rule->Rule << '\t';
			os << rule->Div << '\t';
			os << std::endl;
		}

		os << ptr->ItemsCount << std::endl;
		for (int i = 0; i < ptr->ItemsCount; i++)
		{
			MIX_ITEM * item = &ptr->Items[i];
			os << item->StartID << '\t';
			os << item->EndID << '\t';
			os << item->MinLvl << '\t';
			os << item->MaxLvl << '\t';
			os << item->MinOpt << '\t';
			os << item->MaxOpt << '\t';
			os << item->MinDur << '\t';
			os << item->MaxDur << '\t';
			os << item->MinCount << '\t';
			os << item->MaxCount << '\t';
			os << item->Type << '\t';
			os << std::endl;
		}

		os << "//=================================================================================================" << std::endl;
	}
}

template<int NGROUP>
void MixBmdT<NGROUP>::TxtIn(std::ifstream & is)
{
	assert(is);

	std::string line;
	size_t size = sizeof(MIX_INFO);
	size_t counts[NGROUP] { 0 };

	std::map<int, MIX_INFO> m;
	size_t n = 0;
	while (getline(is, line))
	{
		if (line[0] == '/' && line[1] == '/')
			continue;

		MIX_INFO temp {};	// zeroed: unused Rules/Items and padding were stack garbage before
		int group;

		sscanf(line.c_str(),
			"%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%hhd\t%d\t%hhd\t%hhd\t%hhd\t%hhd\t%hhd\t%hhd"
			, &group, &temp.ID, &temp.Type, &temp.Desc_Msg_1, &temp.Desc_Msg_2, &temp.Desc_Msg_3, &temp.Bad_Msg_1, &temp.Bad_Msg_2, &temp.Bad_Msg_3
			, &temp.Good_Msg_1, &temp.Good_Msg_2, &temp.Good_Msg_3, &temp.Height, &temp.Width, &temp.MinLevel, &temp.MaxSuccesRate, &temp.MoneyRule, &temp.MoneyReq
			, &temp.Unk_1, &temp.Unk_2, &temp.Unk_3, &temp.Unk_4, &temp.Unk_5, &temp.Unk_6
		);

		if (group < 0 || group >= NGROUP) 
		{
			std::cout << "MixBmd::TxtIn (group < 0 || group >= NGROUP)" << std::endl;
			return;
		}

		getline(is, line);
		{
			sscanf(line.c_str(), "%d", &temp.RulesCount);
			if (temp.RulesCount < 0 || temp.RulesCount > 32)
			{
				std::cout << "MixBmd::TxtIn (RulesCount < 0 || RulesCount > 32)" << std::endl;
				return;
			}

			for (int i = 0; i < temp.RulesCount; i++)
			{
				getline(is, line);
				sscanf(line.c_str(), "%d\t%f", &temp.Rules[i].Rule, &temp.Rules[i].Div);
			}
		}

		getline(is, line);
		{
			sscanf(line.c_str(), "%d", &temp.ItemsCount);
			if (temp.ItemsCount < 0 || temp.ItemsCount > 8)
			{
				std::cout << "MixBmd::TxtIn (ItemsCount < 0 || ItemsCount > 8)" << std::endl;
				return;
			}

			for (int i = 0; i < temp.ItemsCount; i++)
			{
				getline(is, line);
				sscanf(line.c_str(), "%hd\t%hd\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d"
					, &temp.Items[i].StartID, &temp.Items[i].EndID, &temp.Items[i].MinLvl, &temp.Items[i].MaxLvl, &temp.Items[i].MinOpt
					, &temp.Items[i].MaxOpt, &temp.Items[i].MinDur, &temp.Items[i].MaxDur, &temp.Items[i].MinCount, &temp.Items[i].MaxCount, &temp.Items[i].Type);
			}
		}


		int key = group << 16 | temp.ID;
		if (m.find(key) == m.end())
		{
			counts[group]++;
			n++;
			m[key] = temp;
		}
		else
		{
			std::cout << " Duplicated Key : " << group << " " << temp.ID << std::endl;
		}
	}

	size_t buf_size = NGROUP * 4 + (n * size);
	this->_buf.resize(buf_size);
	memcpy(&this->_buf[0], counts, NGROUP * 4);
	size_t pos = NGROUP * 4;

	if (n > 0)
	{
		for (auto it = m.begin(); it != m.end(); it++)
		{
			memcpy(&this->_buf[pos], &it->second, size);
			pos += size;
		}
	}

}

template<> inline const char* MixBmdT<13>::Header() { return ""; }
template<> inline const char* MixBmdT<14>::Header() { return "//GMO mix.bmd (14 groups)"; }

typedef MixBmdT<13> MixBmd;
typedef MixBmdT<14> MixBmdGMO;

#endif
