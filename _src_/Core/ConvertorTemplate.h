#ifndef CONVERTOR_TEMPLATE_H
#define CONVERTOR_TEMPLATE_H

template <class Convertor, const char* EXT_1, const char* EXT_2>
int _Convert(int argc, char** argv)
{
	Convertor convertor;
	const char* szInputPath = nullptr;
	const char* szOutputPath = nullptr;

	if (argc >= 2)
		szInputPath = argv[1];
	if (argc >= 3)
		szOutputPath = argv[2];

	if (!szInputPath)
	{
		PRINT_DEBUG("Drag&Drop the '.bmd' / '.txt' file \n" "\t or use console command to execute with the file path.");
		return EXIT_FAILURE;
	}

	if (!fs::exists(szInputPath))
	{
		PRINT_DEBUG("[ERROR] Input file does not exist.");
		return EXIT_FAILURE;
	}

	if (fs::is_regular_file(szInputPath))
	{
		auto ext = fs::path(szInputPath).extension().string();
		Utls::ToLowerCaseString(ext);
		if (ext == EXT_1)
		{
			if (!convertor.Unpack(szInputPath, szOutputPath))
			{
				return EXIT_FAILURE;
			}
		}
		else if (ext == EXT_2)
		{
			if (!convertor.Pack(szInputPath, szOutputPath))
			{
				return EXIT_FAILURE;
			}
		}
		else
		{
			PRINT_DEBUG("[ERROR] File ext is not '.bmd' / '.txt'.");
			return EXIT_FAILURE;
		}
	}
	else
	{
		PRINT_DEBUG("[ERROR] Invalid file path.");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

constexpr const char DEFAULT_EXT_1[] = ".bmd";
constexpr const char DEFAULT_EXT_2[] = ".txt";

template <class Convertor>
int Convert(int argc, char** argv)
{
	return _Convert<Convertor, DEFAULT_EXT_1, DEFAULT_EXT_2>(argc, argv);
}

// ---- Two client versions of the same file (e.g. Season 16 and GMO) ----

// Lower-case extension of the input file ("" if there is no input file).
inline std::string AutoInputExt(int argc, char** argv)
{
	if (argc < 2 || !fs::is_regular_file(argv[1])) return "";
	auto ext = fs::path(argv[1]).extension().string();
	Utls::ToLowerCaseString(ext);
	return ext;
}

inline size_t AutoInputSize(int argc, char** argv)
{
	return argc >= 2 ? (size_t)fs::file_size(argv[1]) : 0;
}

// Is one of the leading comment lines of the input txt equal to 'header'?
// (some tools write extra "// ..." lines above the field header)
inline bool AutoTxtHasHeader(int argc, char** argv, std::string header)
{
	while (!header.empty() && (header.back() == '\n' || header.back() == '\r'))
		header.pop_back();
	std::ifstream is(argv[1], std::ios::in | std::ios::binary);
	std::string line;
	for (int n = 0; n < 64 && std::getline(is, line); n++)
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		if (line == header || line == "//" + header)
			return true;
		if (line.compare(0, 2, "//") != 0)
			break;	// past the comment block
	}
	return false;
}

template <class ConvA, class ConvB>
int ConvertPick(int argc, char** argv, bool useB, const char* nameA = "S16", const char* nameB = "GMO")
{
	std::cout << "Format: " << (useB ? nameB : nameA) << '\n';
	return useB ? _Convert<ConvB, DEFAULT_EXT_1, DEFAULT_EXT_2>(argc, argv)
				: _Convert<ConvA, DEFAULT_EXT_1, DEFAULT_EXT_2>(argc, argv);
}

// Plain TxtConvertor files:
// .bmd: [count][count x record][CRC] -> the version whose record size matches the file size.
// .txt: the version whose header line (//Field1<TAB>Field2...) is in the leading comment lines.
// Anything else goes to ConvA (the Season 16 behaviour as before).
template <class ConvA, class ConvB>
int ConvertAuto(int argc, char** argv, const char* nameA = "S16", const char* nameB = "GMO")
{
	bool useB = false;
	const std::string ext = AutoInputExt(argc, argv);
	if (ext == DEFAULT_EXT_1)
	{
		std::ifstream is(argv[1], std::ios::in | std::ios::binary);
		DWORD count = 0;
		is.read((char*)&count, 4);
		useB = AutoInputSize(argc, argv) == 8 + (size_t)count * sizeof(typename ConvB::Record);
	}
	else if (ext == DEFAULT_EXT_2)
		useB = AutoTxtHasHeader(argc, argv, ConvB::Record::GetLabel());
	return ConvertPick<ConvA, ConvB>(argc, argv, useB, nameA, nameB);
}

// Files with a fixed size (no counter): ConvB::FILE_SIZE for .bmd, ConvB::Header() for .txt.
template <class ConvA, class ConvB>
int ConvertAutoFixed(int argc, char** argv, const char* nameA = "S16", const char* nameB = "GMO")
{
	bool useB = false;
	const std::string ext = AutoInputExt(argc, argv);
	if (ext == DEFAULT_EXT_1)
		useB = AutoInputSize(argc, argv) == ConvB::FILE_SIZE;
	else if (ext == DEFAULT_EXT_2)
		useB = AutoTxtHasHeader(argc, argv, ConvB::Header());
	return ConvertPick<ConvA, ConvB>(argc, argv, useB, nameA, nameB);
}

#endif // !CONVERTOR_TEMPLATE_H
