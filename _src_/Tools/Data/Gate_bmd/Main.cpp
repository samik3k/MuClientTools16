// This file contains the 'main' function. Program execution begins and ends there.
// Requirement: C++17
// GMO (current Global MU client): Gate.bmd (GMO, 512 x 14 bytes)
#include "Core.h"
#include "GmoBmd.h"

int main(int argc, char** argv)
{
	return Convert<GateGmoBmd>(argc, argv);
}
