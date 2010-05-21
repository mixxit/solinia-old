#ifndef UT_MISC_PACK_FILE
#define UT_MISC_PACK_FILE


// Commenting out the ifdef since the files are authored on Windows
// and therefore always have a Windows-style newline.
//#ifdef NL_OS_WINDOWS
const string NewLine("\r\n");
//#elif defined(NL_OS_UNIX)
//const string NewLine("\n");
//#else
//#error "Specify the new line format for text file";
//#endif

#ifndef NEL_UNIT_BASE
#define NEL_UNIT_BASE ""
#endif // NEL_UNIT_BASE

// Test suite for bnp and xml pack files
class CUTMiscPackFile : public Test::Suite
{
	string		_WorkingPath;
	string		_OldPath;
public:
	CUTMiscPackFile ()
	{
		TEST_ADD(CUTMiscPackFile::addBnp);
		TEST_ADD(CUTMiscPackFile::loadFromBnp);
		TEST_ADD(CUTMiscPackFile::addXmlpack);
		TEST_ADD(CUTMiscPackFile::loadFromXmlpack);
		TEST_ADD(CUTMiscPackFile::compressMemory);
		TEST_ADD(CUTMiscPackFile::loadFromBnpCompressed);
		TEST_ADD(CUTMiscPackFile::loadFromXmlpackCompressed);
		TEST_ADD(CUTMiscPackFile::decompressMemory);
		TEST_ADD(CUTMiscPackFile::loadFromBnpUncompressed);
		TEST_ADD(CUTMiscPackFile::loadFromXmlpackUncompressed);
		TEST_ADD(CUTMiscPackFile::loadXmlpackWithSameName);
	}

	void setup()
	{
		_OldPath = CPath::getCurrentPath();
		CPath::setCurrentPath(_WorkingPath.c_str());
		string pathAfter = CPath::getCurrentPath();
	}

	void tear_down()
	{
		CPath::setCurrentPath(_OldPath.c_str());
	}

	void addBnp()
	{
		// add bnp file in the path and access to file inside
		CPath::addSearchBigFile(NEL_UNIT_BASE "ut_misc_files/files.bnp", false, false);
	}

	void loadFromBnp()
	{
		// lookup for the file
		string filename = CPath::lookup("file1_in_bnp.txt", true, true, false);
		TEST_ASSERT(filename == "files.bnp@file1_in_bnp.txt");

		// read the first file content
		{
			CIFile file1(filename);
			string content1;
			content1.resize(file1.getFileSize());
			file1.serialBuffer((uint8*)content1.data(), file1.getFileSize());
			
			// check the file content
			TEST_ASSERT(content1 == "The content of the first file");
		}

		// lookup for the 2nd file
		filename = CPath::lookup("file2_in_bnp.txt", true, true, false);
		TEST_ASSERT(filename == "files.bnp@file2_in_bnp.txt");

		{
			// read the second file content
			CIFile file2(filename);
			string content2;
			content2.resize(file2.getFileSize());
			file2.serialBuffer((uint8*)content2.data(), file2.getFileSize());
			
			// check the file content
			TEST_ASSERT(content2 == "Another content but for the second file");
		}
	}

	void addXmlpack()
	{
		// add xml_pack file in the path and access to file inside
		CPath::addSearchXmlpackFile(NEL_UNIT_BASE "ut_misc_files/xml_files/xml_files.xml_pack", false, false);
	}

	void loadFromXmlpack()
	{
		// lookup for the file
		string filename = CPath::lookup("file1_in_xml_pack.xml", true, true, false);
		TEST_ASSERT(filename == NEL_UNIT_BASE "ut_misc_files/xml_files/xml_files.xml_pack@@file1_in_xml_pack.xml");

		// read the first file content
		{
			CIFile file1(filename);
			string content1;
			content1.resize(file1.getFileSize());
			file1.serialBuffer((uint8*)content1.data(), file1.getFileSize());
			
			// check the file content
			string refText = "<myxml><withSomethink name=\"foo\"/></myxml>"+NewLine;
			TEST_ASSERT(content1 == refText);
		}

		// lookup for the 2nd file
		filename = CPath::lookup("file2_in_xml_pack.xml", true, true, false);
		TEST_ASSERT(filename == NEL_UNIT_BASE "ut_misc_files/xml_files/xml_files.xml_pack@@file2_in_xml_pack.xml");

		{
			// read the second file content
			CIFile file2(filename);
			string content2;
			content2.resize(file2.getFileSize());
			file2.serialBuffer((uint8*)content2.data(), file2.getFileSize());
			
			// check the file content
			string refText="<anotherxml><withSomethink name=\"bar\"/></anotherxml>"+NewLine;
			TEST_ASSERT(content2 == refText);
		}
	}

	void compressMemory()
	{
//#ifdef WIN32
//_CrtCheckMemory();
//#endif
		CPath::memoryCompress();
//#ifdef WIN32
//_CrtCheckMemory();
//#endif
	}

	void loadFromBnpCompressed()
	{
		// simply recall loadFromBnp
		loadFromBnp();
	}

	void loadFromXmlpackCompressed()
	{
		// simply recall loadFromXmlpack
		loadFromXmlpack();
	}

	void decompressMemory()
	{
		CPath::memoryUncompress();
	}

	void loadFromBnpUncompressed()
	{
		// simply recall loadFromBnp
		loadFromBnp();
	}

	void loadFromXmlpackUncompressed()
	{
		// simply recall loadFromXmlpack
		loadFromXmlpack();
	}

	void loadXmlpackWithSameName()
	{
		// we support xml_pack file in subfolder that have the same name
		// but the 'addSearchPath' or add xml pack must be done
		// at a higher discriminant directory

//		CPath::addSearchXmlpackFile(NEL_UNIT_BASE "ut_misc_files/xml_files/same_subfolder_1/samename/samename.xml_pack", true, false, NULL);
//		CPath::addSearchXmlpackFile(NEL_UNIT_BASE "ut_misc_files/xml_files/same_subfolder_2/samename/samename.xml_pack", true, false, NULL);
		CPath::addSearchPath(NEL_UNIT_BASE "ut_misc_files/xml_files", true, false);

		// lookup for the files in first subdirectory
		string filename = CPath::lookup("file1_in_sub_1.xml", true, true, false);
		TEST_ASSERT(filename == NEL_UNIT_BASE "ut_misc_files/xml_files/same_subfolder_1/samename/samename.xml_pack@@file1_in_sub_1.xml");
		filename = CPath::lookup("file2_in_sub_1.xml", true, true, false);
		TEST_ASSERT(filename == NEL_UNIT_BASE "ut_misc_files/xml_files/same_subfolder_1/samename/samename.xml_pack@@file2_in_sub_1.xml");

		// lookup for the files in the second subdirectory
		filename = CPath::lookup("file1_in_sub_2.xml", true, true, false);
		TEST_ASSERT(filename == NEL_UNIT_BASE "ut_misc_files/xml_files/same_subfolder_2/samename/samename.xml_pack@@file1_in_sub_2.xml");
		filename = CPath::lookup("file2_in_sub_2.xml", true, true, false);
		TEST_ASSERT(filename == NEL_UNIT_BASE "ut_misc_files/xml_files/same_subfolder_2/samename/samename.xml_pack@@file2_in_sub_2.xml");

		// read the file content of the first file in first pack
		filename = CPath::lookup("file1_in_sub_1.xml", true, true, false);

		// check that we can read the file modif date
		uint32 d = NLMISC::CFile::getFileModificationDate(filename);
		TEST_ASSERT(d != 0);
		{
			CIFile file1(filename);
			string content1;
			content1.resize(file1.getFileSize());
			file1.serialBuffer((uint8*)content1.data(), file1.getFileSize());
			
			// check the file content
			string refText = "<myxml><withSomethink name=\"foo\"/></myxml>"+NewLine;
			TEST_ASSERT(content1 == refText);
		}

		// read the file content of the second file in the second pack
		filename = CPath::lookup("file2_in_sub_2.xml", true, true, false);
		{
			// read the second file content
			CIFile file2(filename);
			string content2;
			content2.resize(file2.getFileSize());
			file2.serialBuffer((uint8*)content2.data(), file2.getFileSize());
			
			// check the file content
			string refText="<anotherxml><withSomethink name=\"bar\"/></anotherxml>"+NewLine;
			TEST_ASSERT(content2 == refText);
		}

	}

};

#endif
