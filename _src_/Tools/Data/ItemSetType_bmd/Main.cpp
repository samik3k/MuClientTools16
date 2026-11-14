// This file contains the 'main' function. Program execution begins and ends there.
// Requirement: C++17
#include "Core.h"
#include "ItemSetBmd.h"

int main(int argc, char** argv)
{
	// Season 16 or GMO itemsettype.bmd / .txt - detected from the file
	return ConvertAutoFixed<ItemSetTypeBmd, ItemSetTypeBmdGMO>(argc, argv);
}