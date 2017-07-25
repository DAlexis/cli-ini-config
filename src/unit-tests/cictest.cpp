#include "cic.hpp"

#include "gtest/gtest.h"

#include <fstream>
#include <sstream>
#include <cstdio>

using namespace cic;
using namespace std;

const char testConfigFilename[] = "test-config.ini";

bool createTestIniFile()
{
	const char iniFile[] =
		"# Some strange parameters\n"
		"# Not all used by every test\n"

		"[TestingParameters]\n"
		"bool-parameter = true\n"
		"int-parameter = 321\n"
		"double-parameter = -3.12e10\n"
		"string-parameter = hello\n"

		"[Group1]\n"
		"bool-parameter = false\n"
		"int-parameter = 1\n"
		"double-parameter = -4.2e10\n"
		"string-parameter = bye\n"

		"[Group2]\n"
		"bool-parameter = true\n"
		"int-parameter = 2\n"
		"double-parameter = 1309.1\n"
		"string-parameter = lol\n"
		;

	ofstream f(testConfigFilename, ios::out);
	if (!f.good())
		return false;

	f << iniFile;
	return true;
}

void removeTestIniFile()
{
	std::remove(testConfigFilename);
}

struct Remover {
	~Remover() { removeTestIniFile(); }
};

TEST(Parameter, Instantiation)
{
    ASSERT_NO_THROW(Parameter<int>("test", "Test parameter1"));
    ASSERT_NO_THROW(Parameter<int>("test", "Test parameter2", 28));

    ASSERT_NO_THROW(Parameter<double>("test", "Test parameter3", -123e76));
    ASSERT_NO_THROW(Parameter<std::string>("test", "Test parameter4", "Value"));
}

TEST(Parameter, ProgramOption)
{
	namespace po = boost::program_options;
    Parameter<int> p1("par1", "Test parameter1", 25);
    po::options_description options("Options");
    ASSERT_NO_THROW(p1.addToPO(options));

    ASSERT_TRUE(p1.initialized());
    ASSERT_FALSE(p1.setByUser());

    int argc = 2;
    const char *argv[] = {"/bin/myprogram", "--par1=30"};
    boost::program_options::variables_map co;

	ASSERT_NO_THROW( po::store(po::parse_command_line(argc, argv, options), co) );
	ASSERT_NO_THROW( po::notify(co) );

	ASSERT_NO_THROW( p1.getFromPO(co) );

	ASSERT_TRUE(p1.initialized());
	ASSERT_TRUE(p1.setByUser());


	ASSERT_EQ(p1.get(), 30);
}

TEST(ParametersGrop, AddGet)
{
	ASSERT_NO_THROW(ParametersGroup("Test empty group"));

	ParametersGroup pg1("Test group for adding");
	pg1.add(Parameter<bool>("bool-parameter", "Boolean parameter"));

	int pi = 123;
	double pd = 3.1415926;
	std::string ps = "lol wut";

	pg1.add(
		Parameter<int>("int-parameter", "Integer parameter", pi),
		Parameter<double>("double-parameter", "Double parameter", pd),
		Parameter<std::string>("string-parameter", "String parameter", ps)
	);

	ASSERT_NO_THROW(pg1.get<int>("int-parameter"));
	ASSERT_ANY_THROW(pg1.get<int>("unknown-parameter-name"));

	ASSERT_EQ(pg1.get<int>("int-parameter"), pi);
	ASSERT_EQ(pg1.get<double>("double-parameter"), pd);
	ASSERT_EQ(pg1.get<std::string>("string-parameter"), ps);
}

class ParametersGroupIO : public ::testing::Test
{
public:
	ParametersGroupIO() { }

	void fillWithDefaults()
	{
		pg.add(
			Parameter<bool>("bool-parameter", "Boolean parameter", pb),
			Parameter<int>("int-parameter", "Integer parameter", pi),
			Parameter<double>("double-parameter", "Double parameter", pd),
			Parameter<std::string>("string-parameter", "String parameter", ps)
		);
	}

	void fillNoDefaults()
	{
		pg.add(
			Parameter<bool>("bool-parameter", "Boolean parameter"),
			Parameter<int>("int-parameter", "Integer parameter"),
			Parameter<double>("double-parameter", "Double parameter"),
			Parameter<std::string>("string-parameter", "String parameter")
		);
	}

	ParametersGroup pg{"TestingParameters"};

	bool pb = false;
	int pi = 123;
	double pd = 3.1415926;
	float pf = 2.7182;
	std::string ps = "lol wut";

};

TEST_F(ParametersGroupIO, ParseConsoleOnlyParameresGroup)
{
	namespace po = boost::program_options;
	ASSERT_NO_THROW(fillNoDefaults());
	int argc = 5;
	const char* argv[5];
	argv[0] = "/tmp/test";
	argv[1] = "--bool-parameter";
	argv[2] = "--int-parameter=321";
	argv[3] = "--double-parameter=-32.12";
	argv[4] = "--string-parameter=hello";

	ASSERT_TRUE(pg.getInterface("bool-parameter").initialized());
	ASSERT_FALSE(pg.getInterface("int-parameter").initialized());
	ASSERT_FALSE(pg.getInterface("double-parameter").initialized());
	ASSERT_FALSE(pg.getInterface("string-parameter").initialized());

	po::variables_map vmOptions;

	ASSERT_NO_THROW(po::store(po::parse_command_line(argc, argv, pg.getOptionsDesctiption(false)), vmOptions));
	ASSERT_NO_THROW(po::notify(vmOptions));
	ASSERT_NO_THROW(pg.readPOVarsMap(vmOptions));

	EXPECT_TRUE(pg.getInterface("bool-parameter").initialized());
	EXPECT_TRUE(pg.getInterface("int-parameter").initialized());
	EXPECT_TRUE(pg.getInterface("double-parameter").initialized());
	EXPECT_TRUE(pg.getInterface("string-parameter").initialized());

	EXPECT_EQ(pg.get<bool>("bool-parameter"), true);
	EXPECT_EQ(pg.get<int>("int-parameter"), 321);
	EXPECT_EQ(pg.get<double>("double-parameter"), -32.12);
	EXPECT_EQ(pg.get<std::string>("string-parameter"), "hello");
}

TEST_F(ParametersGroupIO, ParseConsole)
{
	namespace po = boost::program_options;
	ASSERT_NO_THROW(fillNoDefaults());
	int argc = 5;
	const char* argv[5];
	argv[0] = "/tmp/test";
	argv[1] = "--bool-parameter";
	argv[2] = "--int-parameter=321";
	argv[3] = "--double-parameter=-32.12";
	argv[4] = "--string-parameter=hello";

	Parameters p;
	ASSERT_NO_THROW(p.addGroup(pg));
	ASSERT_NO_THROW(p.parseCmdline(argc, argv));

	EXPECT_EQ(p["TestingParameters"].get<bool>("bool-parameter"), true);
	EXPECT_EQ(p["TestingParameters"].get<int>("int-parameter"), 321);
	EXPECT_EQ(p["TestingParameters"].get<double>("double-parameter"), -32.12);
	EXPECT_EQ(p["TestingParameters"].get<std::string>("string-parameter"), "hello");
}

TEST_F(ParametersGroupIO, ParseIni)
{
	fillNoDefaults();

	ASSERT_TRUE(createTestIniFile()) << "Cannot create test ini file";
	Remover r;

	Parameters p;
	ASSERT_NO_THROW(p.addGroup(pg));
	ASSERT_NO_THROW(p.parseIni(testConfigFilename));

	EXPECT_EQ(p["TestingParameters"].get<bool>("bool-parameter"), true);
	EXPECT_EQ(p["TestingParameters"].get<int>("int-parameter"), 321);
	EXPECT_EQ(p["TestingParameters"].get<double>("double-parameter"), -3.12e10);
	EXPECT_EQ(p["TestingParameters"].get<std::string>("string-parameter"), "hello");
}

TEST(Parameters, RightRefAdding)
{
	ASSERT_NO_THROW(
		ParametersGroup pg(
			"TestingParameters",
			Parameter<bool>("bool-parameter", "Boolean parameter"),
			Parameter<int>("int-parameter", "Integer parameter")
		)
	);

	Parameters p;
	ASSERT_NO_THROW(
		p.addGroup(
			ParametersGroup(
				"TestingParameters",
				Parameter<bool>("bool-parameter", "Boolean parameter"),
				Parameter<int>("int-parameter", "Integer parameter")
			)
		)
	);

	// Test for addGroup method
	{
		ParametersGroup pg(
			"TestingParameters1",
			Parameter<bool>("bool-parameter", "Boolean parameter"),
			Parameter<int>("int-parameter", "Integer parameter")
		);

		Parameters p;
		p.addGroup(
			pg,
			static_cast<ParametersGroup&&>(ParametersGroup(
				"OtherGroup",
				Parameter<bool>("bool-parameter", "Boolean parameter"),
				Parameter<int>("int-parameter", "Integer parameter")
			)),
			static_cast<ParametersGroup&&>(ParametersGroup(
				"OtherGroup2",
				Parameter<bool>("bool-parameter", "Boolean parameter"),
				Parameter<int>("int-parameter", "Integer parameter")
			))
		);
	}
}

class ParametersShortInit : public ::testing::Test
{
public:
	Parameters p{
		"All parameters for your program",
		ParametersGroup(
			"Group1",
			"Here is a sample group 1",
			Parameter<bool>("bool-parameter", "Boolean parameter"),
			Parameter<int>("int-parameter", "Integer parameter")
		),
		ParametersGroup(
			"Group2",
			//"Here is a sample group 2", // I don't want the description
			Parameter<double>("double-parameter", "Double parameter", 1.23),
			Parameter<std::string>("string-parameter", "String parameter", "lol wut2")
		),
		ParametersGroup(
			"Group3",
			"Another sample group",
			Parameter<std::string>("string-parameter-with-other-default", "String parameter", "wut lol")
		)
	};
};

TEST_F(ParametersShortInit, Cmdline)
{
	int argc = 5;
	const char* argv[5];
	argv[0] = "/tmp/test";
	argv[1] = "--bool-parameter";
	argv[2] = "--int-parameter=321";
	argv[3] = "--double-parameter=-32.12";
	argv[4] = "--string-parameter=hello";

	ASSERT_NO_THROW(p.parseCmdline(argc, argv));

	EXPECT_EQ(p["Group1"].get<bool>("bool-parameter"), true);
	EXPECT_EQ(p["Group1"].get<int>("int-parameter"), 321);
	EXPECT_EQ(p["Group2"].get<double>("double-parameter"), -32.12);
	EXPECT_EQ(p["Group2"].get<std::string>("string-parameter"), "hello");
}

TEST_F(ParametersShortInit, CmdlineBoolParse)
{
	int argc = 2;
	const char* argv[argc];
	argv[0] = "/tmp/test";
	argv[1] = "--bool-parameter";

	ASSERT_NO_THROW(p.parseCmdline(argc, argv));
	EXPECT_EQ(p["Group1"].get<bool>("bool-parameter"), true);

	// Tests below could not run because --bool-parameter=true is not supported
	/*
	argv[1] = "--bool-parameter=false";
	ASSERT_NO_THROW(p.parseCmdline(argc, argv));
	EXPECT_EQ(p["Group1"].get<bool>("bool-parameter"), false);

	argv[1] = "--bool-parameter=true";
	ASSERT_NO_THROW(p.parseCmdline(argc, argv));
	EXPECT_EQ(p["Group1"].get<bool>("bool-parameter"), true);
	*/
}

TEST_F(ParametersShortInit, IniFile)
{
	ASSERT_TRUE(createTestIniFile()) << "Cannot create test ini file";
	Remover r;

	ASSERT_NO_THROW(p.parseIni(testConfigFilename));

	EXPECT_EQ(p["Group1"].get<bool>("bool-parameter"), false);
	EXPECT_EQ(p["Group1"].get<int>("int-parameter"), 1);
	EXPECT_EQ(p["Group2"].get<double>("double-parameter"), 1309.1);
	EXPECT_EQ(p["Group2"].get<std::string>("string-parameter"), "lol");
}

TEST_F(ParametersShortInit, CmdlineOverridesIni)
{
	ASSERT_TRUE(createTestIniFile()) << "Cannot create test ini file";
	Remover r;

	constexpr int argc = 4;
	const char* argv[argc];
	argv[0] = "/tmp/test";
	argv[1] = "--bool-parameter";
	argv[2] = "--int-parameter=321";
	argv[3] = "--double-parameter=-32.12";

	ASSERT_NO_THROW(p.parseIni(testConfigFilename));

	EXPECT_EQ(p["Group2"].get<std::string>("string-parameter"), "lol") << "Ini parsing does not work";

	ASSERT_NO_THROW(p.parseCmdline(argc, argv));

	EXPECT_EQ(p["Group2"].get<std::string>("string-parameter"), "lol") << "Cmdline parsing broke other parameter";

	EXPECT_EQ(p["Group1"].get<bool>("bool-parameter"), true);
	EXPECT_EQ(p["Group1"].get<int>("int-parameter"), 321);
	EXPECT_EQ(p["Group2"].get<double>("double-parameter"), -32.12);
}

TEST_F(ParametersShortInit, CmdlineWithGroups)
{
	Remover r;

	constexpr int argc = 4;
	const char* argv[argc];
	argv[0] = "/tmp/test";
	argv[1] = "--Group1.bool-parameter";
	argv[2] = "--int-parameter=321";
	argv[3] = "--Group2.double-parameter=-32.12";

	ASSERT_NO_THROW(p.parseCmdline(argc, argv, true, true));

	EXPECT_EQ(p["Group1"].get<bool>("bool-parameter"), true);
	EXPECT_EQ(p["Group1"].get<int>("int-parameter"), 321);
	EXPECT_EQ(p["Group2"].get<double>("double-parameter"), -32.12);
}


TEST_F(ParametersShortInit, TextGeneration)
{
	{
		std::ostringstream oss;
		EXPECT_NO_THROW(p.cmdlineHelp(oss, false));
		EXPECT_FALSE(oss.str().empty());
		EXPECT_NE(oss.str().find("lol wut"), string::npos) << "Default value was not shown in help";
		//std::cout << oss.str();
	}
	{
		std::ostringstream oss;
		EXPECT_NO_THROW(p.cmdlineHelp(oss, true));
		EXPECT_FALSE(oss.str().empty());
		EXPECT_NE(oss.str().find("lol wut"), string::npos) << "Default value was not shown in help";
		//std::cout << oss.str();
	}
	{
		std::ostringstream oss;
		EXPECT_NO_THROW(p.writeIni(oss));
		EXPECT_FALSE(oss.str().empty());
		EXPECT_NE(oss.str().find("lol wut"), string::npos) << "Default value was not shown in ini file";
		//std::cout << oss.str();
	}
}

TEST(ParametersOptions, ParameterType)
{
	ASSERT_TRUE(createTestIniFile()) << "Cannot create test ini file";
	Remover r;

	Parameters p(
		"All parameters for your program",
		ParametersGroup(
			"Group1",
			Parameter<bool>("bool-parameter", "Boolean parameter"),
			Parameter<int>("int-parameter", "Integer parameter", ParamterType::iniFile)
		),
		ParametersGroup(
			"Group2",
			Parameter<double>("double-parameter", "Double parameter", 1.23, ParamterType::cmdLine),
			Parameter<std::string>("string-parameter", "String parameter", ParamterType::both)
		)
	);

	constexpr int argc = 4;
	const char* argv[argc];
	argv[0] = "/tmp/test";
	argv[1] = "--string-parameter=wut";
	argv[2] = "--bool-parameter";
	argv[3] = "--double-parameter=-32.12";

	EXPECT_EQ(p["Group2"].get<double>("double-parameter"), 1.23);

	ASSERT_NO_THROW(p.parseIni(testConfigFilename));

	// All whould be according ini file
	EXPECT_EQ(p["Group1"].get<int>("int-parameter"), 1);
	EXPECT_EQ(p["Group2"].get<std::string>("string-parameter"), "lol");

	ASSERT_NO_THROW(p.parseCmdline(argc, argv));

	EXPECT_EQ(p["Group1"].get<int>("int-parameter"), 1);
	EXPECT_EQ(p["Group2"].get<double>("double-parameter"), -32.12); // As in console
	EXPECT_EQ(p["Group2"].get<std::string>("string-parameter"), "wut"); // Changed

	ASSERT_NO_THROW(p.parseIni(testConfigFilename));

	EXPECT_EQ(p["Group1"].get<int>("int-parameter"), 1);
	EXPECT_EQ(p["Group2"].get<double>("double-parameter"), -32.12); // Not changed
	EXPECT_EQ(p["Group2"].get<std::string>("string-parameter"), "lol"); // Changed

	argv[1] = "--int-parameter=45";
	ASSERT_ANY_THROW(p.parseCmdline(argc, argv)) << "Non-cmdline parameter should throw an exception when found in cmdline";
}

