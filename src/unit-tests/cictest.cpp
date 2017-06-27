#include "cic.hpp"

#include "gtest/gtest.h"

using namespace cic;

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

	ParametersGroup pg{"Parameters group for testing"};

	bool pb = false;
	int pi = 123;
	double pd = 3.1415926;
	float pf = 2.7182;
	std::string ps = "lol wut";

};

TEST_F(ParametersGroupIO, ParseConsole)
{
	namespace po = boost::program_options;
	ASSERT_NO_THROW(fillNoDefaults());
	int argc = 5;
	const char* argv[5];
	argv[0] = "/tmp/test";
	argv[1] = "--bool-parameter=false";
	argv[2] = "--int-parameter=321";
	argv[3] = "--double-parameter=-32.12";
	argv[4] = "--string-parameter=hello";

	ASSERT_FALSE(pg.getInterface("bool-parameter").initialized());
	ASSERT_FALSE(pg.getInterface("int-parameter").initialized());
	ASSERT_FALSE(pg.getInterface("double-parameter").initialized());
	ASSERT_FALSE(pg.getInterface("string-parameter").initialized());

	po::variables_map vmOptions;

	ASSERT_NO_THROW(po::store(po::parse_command_line(argc, argv, pg.getOptionsDesctiption()), vmOptions));
	ASSERT_NO_THROW(po::notify(vmOptions));
	ASSERT_NO_THROW(pg.readPOVarsMap(vmOptions));

	EXPECT_TRUE(pg.getInterface("bool-parameter").initialized());
	EXPECT_TRUE(pg.getInterface("int-parameter").initialized());
	EXPECT_TRUE(pg.getInterface("double-parameter").initialized());
	EXPECT_TRUE(pg.getInterface("string-parameter").initialized());

	EXPECT_EQ(pg.get<bool>("bool-parameter"), false);
	EXPECT_EQ(pg.get<int>("int-parameter"), 321);
	EXPECT_EQ(pg.get<double>("double-parameter"), -32.12);
	EXPECT_EQ(pg.get<std::string>("string-parameter"), "hello");
}
