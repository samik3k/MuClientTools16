// Terrain files (Terrain*.att / .map / .obj and EncTerrain*.*) and the Enc1.dat / Dec2.dat packet key files
// of old clients. Every encoding used from 0.68 to 1.04 is detected from the data itself.
#ifndef LEGACY_TERRAIN_H
#define LEGACY_TERRAIN_H

#include "LegacyCommon.h"

namespace Legacy
{
	bool IsTerrainExt(const std::string& lowerExt); // .att .map .obj
	// writes <dstNoExt>.<tta|pam|jbo> (decrypted, original layout) and <dstNoExt>.<att|map|obj>.txt (readable dump)
	bool UnpackTerrain(const fs::path& src, const fs::path& dstNoExt, std::string& msg);

	bool IsKeyFile(const std::string& lowerName); // enc1.dat, dec2.dat
	bool UnpackKeyFile(const fs::path& src, const fs::path& dstNoExt, std::string& msg);
}

#endif // !LEGACY_TERRAIN_H
