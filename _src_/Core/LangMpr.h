
#ifndef LANG_MPR_H
#define LANG_MPR_H

#define _LANG_WKEY_ 0x12DC
#define _LANG_ZIP_PASSWORD_ L"q!h#P<xALqU*sxC@{eHr|N)$:"
// Main Season 16 1.19.46 offset lang.mpr password : 0x0163BEBC

// Known Lang.mpr zip passwords. Unpack tries them in this order and remembers the one that works in
// "<unpacked folder>.mprkey"; Pack reads that file (default: the first entry, Season 16).
// The XOR layer (_MU_XOR3_KEY_ + _LANG_WKEY_) and the CRC are the same in all of these versions.
struct LANG_MPR_KEY
{
	const char* Name;
	const wchar_t* Password;
};

static const LANG_MPR_KEY LANG_MPR_KEYS[] =
{
	{ "S16", _LANG_ZIP_PASSWORD_ },               // Season 16 part 1.1, main 1.19.46
	{ "GMO", L"imeX#BqL]iiC]v]U$4-}+O86i" },      // Global MU Online 1.12.9 (2023); from the Themida-unpacked main.exe
};

class LangMpr : public BaseIO
{
public:
	LangMpr() : _wkey(_LANG_WKEY_), _key(0) {};
	virtual ~LangMpr() {};

	BOOL Unpack(const char *szSrcFile, const char *szDestDir);
	BOOL Pack(const char *szSrcDir, const char *szDestFile);

private:
	BOOL FileOpen(const char *szSrcFile);
	BOOL FileWrite(const char *szDestFile);
	BOOL Decrypt();
	BOOL Encrypt();
	BOOL UnZip(const char *szDestDir);
	BOOL Zip(const char *szSrcDir);

	WORD _wkey;
	size_t _key;	// index into LANG_MPR_KEYS
};

#endif // !LANG_MPR_H
