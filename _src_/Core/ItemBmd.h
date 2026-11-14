
#ifndef ITEM_BMD_H
#define ITEM_BMD_H

#define _ITEM_BMD_WKEY_ _COMMON_TXTCONVERTOR_WKEY_

// Item.bmd for Season 16 (ITEM_ATTRIBUTE, 680 bytes) and GMO (ITEM_ATTRIBUTE_GMO, 692 bytes).
// Item_bmd.exe picks the right one by itself (ConvertAuto).
template<typename T>
class ItemBmdT : public TxtConvertor<T>
{
public:
	ItemBmdT() : TxtConvertor<T>(_ITEM_BMD_WKEY_) {};
	virtual ~ItemBmdT() {};

private:

	int GetKey(T* ptr)
	{
		int check_index = (ptr->ItemSubGroup * 512) + ptr->ItemSubIndex;
		if (ptr->ItemIndex != check_index)
		{
			return -1;	//it will push the problematic row to the top of the out.txt file
		}

		return ptr->ItemIndex;
	};
};

typedef ItemBmdT<ITEM_ATTRIBUTE> ItemBmd;
typedef ItemBmdT<ITEM_ATTRIBUTE_GMO> ItemBmdGMO;

#endif
