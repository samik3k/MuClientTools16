// This file contains the 'main' function. Program execution begins and ends there.
// Requirement: C++17
#include "Core.h"
#include "PentagramMixNeedSourceBmd.h"

int main(int argc, char** argv)
{
	// Season 16 or GMO - detected from the file
	return ConvertAuto<PentagramMixNeedSourceBmd, PentagramMixNeedSourceBmdGMO>(argc, argv);
}