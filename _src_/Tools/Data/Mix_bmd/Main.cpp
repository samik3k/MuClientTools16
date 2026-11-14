// This file contains the 'main' function. Program execution begins and ends there.
// Requirement: C++17
#include "Core.h"
#include "MixBmd.h"

int main(int argc, char** argv)
{
	// Season 16 (13 groups) or GMO (14 groups) mix.bmd / .txt - detected from the file
	bool gmo = false;
	const std::string ext = AutoInputExt(argc, argv);
	if (ext == DEFAULT_EXT_1)
		gmo = MixBmdGMO().MatchBmd(argv[1]);
	else if (ext == DEFAULT_EXT_2)
		gmo = AutoTxtHasHeader(argc, argv, MixBmdGMO::Header());
	return ConvertPick<MixBmd, MixBmdGMO>(argc, argv, gmo);
}