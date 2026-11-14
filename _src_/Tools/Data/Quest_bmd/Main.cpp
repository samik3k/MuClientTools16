// This file contains the 'main' function. Program execution begins and ends there.
// Requirement: C++17
#include "Core.h"
#include "QuestBmd.h"

int main(int argc, char** argv)
{
	// Season 16 or GMO Quest.bmd / .txt - detected from the file
	return ConvertAutoFixed<QuestBmd, QuestBmdGMO>(argc, argv);
}