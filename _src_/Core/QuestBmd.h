
#ifndef QUEST_BMD_H
#define QUEST_BMD_H

#include <sstream>

// Quest.bmd: 200 x QUEST_INFO, every record XOR3, no counter, no CRC.
// Season 16 = QUEST_INFO     (808 bytes: QuestName[32], items with 11 classes)
// GMO       = QUEST_INFO_GMO (872 bytes: QuestName[64], items with 14 classes)
// Quest_bmd.exe picks the version by itself (file size / "//GMO Quest.bmd" first line).
template<typename T>
class QuestBmdT : public TxtConvertor<T>
{
public:
	static const size_t COUNT = 200;
	static const size_t FILE_SIZE = COUNT * sizeof(T);
	static const char* Header();	// first txt line ("" = none, Season 16 as before)

	QuestBmdT() : TxtConvertor<T>() { this->hasCounter = false; this->hasCRC = false; };
	virtual ~QuestBmdT() { };

private:
	void TxtOut(std::ofstream& os)
	{
		assert(os);

		if (*Header())
			os << Header() << std::endl;

		for (auto it = this->_map.begin(); it != this->_map.end(); it++)
		{
			T* ptr = it->second;

			if (ptr->ItemCount + ptr->TypeCount == 0)	//blank
				continue;

			os << "//=================================================" << std::endl;
			os << "//==Quest: " << ptr->QuestName << std::endl;
			os << "//=================================================" << std::endl;
			os << "//ItemCount\tTypeCount\tNPC\tQuestName" << std::endl;
			os << ptr->ItemCount << '\t';
			os << ptr->TypeCount << '\t';
			os << ptr->NPC << '\t';
			os << ptr->QuestName << '\t';
			os << std::endl;

			os << "//QuestItems" << std::endl;
			os << "//Flag\tQuestType\tItemCat\tItemIndex\tLevel\tAmount\tLinkConditionIndex\tDW\tDK\tFE\tMG\tDL\tSU\tRF\tGL\tRW\tSL\tGC";
			if (T::EXTRA_CLASSES)
				os << "\tClass12\tClass13\tClass14";
			os << "\tMsg_1\tMsg_2\tMsg_3\tMsg_4" << std::endl;

			for (int i = 0; i < ptr->ItemCount && i < 16; i++)
			{
				auto& item = ptr->Items[i];
				os << (int)item.Flag << '\t';
				os << (int)item.QuestType << '\t';
				os << (int)item.ItemCat << '\t';
				os << (int)item.ItemIndex << '\t';
				os << (int)item.Level << '\t';
				os << (int)item.Amount << '\t';
				os << (int)item.LinkConditionIndex << '\t';
				for (int c = 0; c < T::CLASSES; c++)
					os << (int)item.Class[c] << '\t';
				os << item.Msg_1 << '\t';
				os << item.Msg_2 << '\t';
				os << item.Msg_3 << '\t';
				os << item.Msg_4 << '\t';
				os << std::endl;
			}

			os << "//QuestTypes" << std::endl;
			os << "//Flag\tType\tQuestIndex\tMinLvl\tMaxLvl\tNeedStr\tNeedZen\tMsg" << std::endl;

			for (int i = 0; i < ptr->TypeCount && i < 16; i++)
			{
				auto& type = ptr->Types[i];
				os << (int)type.Flag << '\t';
				os << (int)type.Type << '\t';
				os << type.QuestIndex << '\t';
				os << type.MinLvl << '\t';
				os << type.MaxLvl << '\t';
				os << type.NeedStr << '\t';
				os << type.NeedZen << '\t';
				os << type.Msg << '\t';
				os << std::endl;
			}

			os << "//" << std::endl;
		}
	}

	void TxtIn(std::ifstream& is)
	{
		assert(is);

		auto& buf = this->_buf;
		std::string line;
		const size_t size = sizeof(T);

		buf.resize(COUNT * size);
		memset(buf.data(), 0x00, buf.size());

		auto isComment = [](const std::string& l) { return l.size() >= 2 && l[0] == '/' && l[1] == '/'; };

		size_t n = 0;
		while (getline(is, line) && n < COUNT)
		{
			if (isComment(line) || line.empty())
				continue;

			T* ptr = (T*)&buf[n * size];
			char name[256] = {};
			int ic = 0, tc = 0, npc = 0;
			sscanf(line.c_str(), "%d\t%d\t%d\t%255[^\t]", &ic, &tc, &npc, name);
			ptr->ItemCount = ic;
			ptr->TypeCount = tc;
			ptr->NPC = npc;
			strncpy(ptr->QuestName, name, sizeof(ptr->QuestName) - 1);

			if (ptr->ItemCount > 16 || ptr->TypeCount > 16)
			{
				std::cout << "Error: Quest Items/Types Count must not > 16." << std::endl;
				return;
			}

			int i = 0;
			while (i < ptr->ItemCount && getline(is, line))
			{
				if (isComment(line))
					continue;

				auto& item = ptr->Items[i];
				int v[7 + 16 + 4] = {};
				const int fields = 7 + T::CLASSES + 4;
				std::istringstream ss(line);
				for (int k = 0; k < fields; k++)
					ss >> v[k];
				item.Flag = v[0];
				item.QuestType = v[1];
				item.ItemCat = v[2];
				item.ItemIndex = v[3];
				item.Level = v[4];
				item.Amount = v[5];
				item.LinkConditionIndex = v[6];
				for (int c = 0; c < T::CLASSES; c++)
					item.Class[c] = v[7 + c];
				item.Msg_1 = v[7 + T::CLASSES + 0];
				item.Msg_2 = v[7 + T::CLASSES + 1];
				item.Msg_3 = v[7 + T::CLASSES + 2];
				item.Msg_4 = v[7 + T::CLASSES + 3];
				i++;
			}

			i = 0;
			while (i < ptr->TypeCount && getline(is, line))
			{
				if (isComment(line))
					continue;

				auto& type = ptr->Types[i];
				int flag = 0, typ = 0, qi = 0, minl = 0, maxl = 0, msg = 0;
				DWORD str = 0, zen = 0;
				sscanf(line.c_str(), "%d\t%d\t%d\t%d\t%d\t%lu\t%lu\t%d", &flag, &typ, &qi, &minl, &maxl, &str, &zen, &msg);
				type.Flag = flag;
				type.Type = typ;
				type.QuestIndex = qi;
				type.MinLvl = minl;
				type.MaxLvl = maxl;
				type.NeedStr = str;
				type.NeedZen = zen;
				type.Msg = msg;
				i++;
			}

			n++;
		}
	}
};

template<> inline const char* QuestBmdT<QUEST_INFO>::Header() { return ""; }
template<> inline const char* QuestBmdT<QUEST_INFO_GMO>::Header() { return "//GMO Quest.bmd"; }

typedef QuestBmdT<QUEST_INFO> QuestBmd;
typedef QuestBmdT<QUEST_INFO_GMO> QuestBmdGMO;

#endif
