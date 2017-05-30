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
