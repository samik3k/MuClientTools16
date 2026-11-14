// This file contains the 'main' function. Program execution begins and ends there.
// Requirement: C++17
#include "Core.h"
#include "ItemBmd.h"

int main(int argc, char** argv)
{
	// Season 16 or GMO item.bmd / item.txt - detected from the file
	return ConvertAuto<ItemBmd, ItemBmdGMO>(argc, argv);
}
