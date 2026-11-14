
#ifndef PENTAGRAM_MIX_SOURCE_NEED_BMD_H
#define PENTAGRAM_MIX_SOURCE_NEED_BMD_H

#define _PENTAGRAM_MIX_SOURCE_NEED_BMD_WKEY_ _COMMON_TXTCONVERTOR_WKEY_

// Season 16 = PENTAGRAM_MIX_SOURCE (144 bytes), GMO = PENTAGRAM_MIX_SOURCE_GMO (188 bytes).
template<typename T>
class PentagramMixNeedSourceBmdT : public TxtConvertor<T>
{
public:
	PentagramMixNeedSourceBmdT() : TxtConvertor<T>(_PENTAGRAM_MIX_SOURCE_NEED_BMD_WKEY_) {};
	virtual ~PentagramMixNeedSourceBmdT() {};

private:
	void MakeLabelEx(std::ofstream& os)
	{
		assert(os);
		os << "// Cat0: Material Combination" << std::endl;
		os << "// Cat1: Errtel" << std::endl;
		os << "// Cat2 : Errtel Level Upgrade" << std::endl;
		os << "// Cat3 : Errtel Rank Upgrade" << std::endl;
		os << "// Cat6: Wing Add Option" << std::endl;
		os << "// Cat7: Wing Option Level Upgrade" << std::endl;
	};
	int GetKey(T* ptr) { return (ptr->Category << 8) | ptr->Index; };
};

typedef PentagramMixNeedSourceBmdT<PENTAGRAM_MIX_SOURCE> PentagramMixNeedSourceBmd;
typedef PentagramMixNeedSourceBmdT<PENTAGRAM_MIX_SOURCE_GMO> PentagramMixNeedSourceBmdGMO;

#endif