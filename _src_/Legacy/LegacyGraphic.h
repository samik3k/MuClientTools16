// Images (OZJ/OZT/OZB/OZP/OZD/OZG) and BMD models from old clients, including the quirks found in
// 0.68 - 1.04 data: TGA data stored as .OZJ, Windows icons stored as .ozt, TGA headers with image type 0,
// height maps with a zeroed BMP header, empty "BMD\0" placeholder models.
#ifndef LEGACY_GRAPHIC_H
#define LEGACY_GRAPHIC_H

#include "LegacyCommon.h"

namespace Legacy
{
	bool IsImageExt(const std::string& lowerExt);
	// writes <dstNoExt>.<real image extension>; msg tells what was detected/repaired
	bool UnpackImage(const fs::path& src, const fs::path& dstNoExt, std::string& msg);

	// BMD model: writes <dstNoExt>.txt (structure summary). v0x0C (map-file encrypted) models are also
	// written decrypted as <dstNoExt>.bmd (v0x0A) so tools that only read plain models can open them.
	bool UnpackModel(const fs::path& src, const fs::path& dstNoExt, std::string& msg);
	bool IsModelFile(const Bytes& d);
}

#endif // !LEGACY_GRAPHIC_H
