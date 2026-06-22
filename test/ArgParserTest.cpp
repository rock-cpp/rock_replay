#include "ArgParser.hpp"

#include <boost/test/unit_test.hpp>

void createCommandLineArgs(char* result[], const std::vector<std::string>& args)
{
    for(size_t i = 0; i < args.size(); i++)
    {
        result[i] = (char*) args[i].c_str();
    }

    result[args.size()] = NULL;
}

BOOST_AUTO_TEST_CASE(TestHelpOutput)
{
    ArgParser argParser;

    const std::vector<std::string> args = {"test"};
    char* argsResult[args.size() + 1];
    createCommandLineArgs(argsResult, args);

    BOOST_TEST(!argParser.parseArguments(args.size(), argsResult));
}

BOOST_AUTO_TEST_CASE(TestWhitelist)
{
    ArgParser argParser;

    const std::vector<std::string> args = {"test", "--whitelist", "trajectory,bar", "../logs/"};
    char* argsResult[args.size() + 1];
    createCommandLineArgs(argsResult, args);

    bool result = argParser.parseArguments(args.size(), argsResult);

    BOOST_TEST(result);
    BOOST_TEST(argParser.whiteListTokens.size() == 2);
    BOOST_TEST(argParser.whiteListTokens[0] == "trajectory");
    BOOST_TEST(argParser.whiteListTokens[1] == "bar");
}

BOOST_AUTO_TEST_CASE(TestRenaming)
{
    ArgParser argParser;

    const std::vector<std::string> args = {"test", "--rename", "foo:bar", "../logs/"};
    char* argsResult[args.size() + 1];
    createCommandLineArgs(argsResult, args);

    bool result = argParser.parseArguments(args.size(), argsResult);

    BOOST_TEST(result);
    BOOST_TEST(argParser.renamings.size() == 1);
    BOOST_TEST(argParser.renamings.at("foo") == "bar");
}
