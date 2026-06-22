#pragma once

#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>

class ArgParser
{
public:
    /**
     * @brief Parses the command line arguments.
     *
     * @param argc: Argument counter.
     * @param argv: Argument values.
     * @return bool True if all positional argument requirements are met, false otherwise.
     */
    bool parseArguments(int argc, char* argv[]);

    std::string prefix;
    std::vector<std::string> whiteListTokens;
    std::vector<std::string> fileNames;
    std::map<std::string, std::string> renamings;
    bool headless = false;
    bool no_exit = false;
    bool quiet = false;

private:
    std::string whiteListInput;
    std::vector<std::string> renamingInput;
    std::vector<std::string> fileArgs;
};
