#include "ArgParser.hpp"

#include "LogFileHelper.hpp"

#include <boost/algorithm/string.hpp>
#include <iostream>

bool ArgParser::parseArguments(int argc, char* argv[])
{
    using namespace boost::program_options;

    options_description desc(
        "Usage: rock-replay2 {logfile|*}.log or folder.\nLogging can be controlled via base-logging variables.\nAvailable options");

    desc.add_options()
        ("help", "show this message")
        ("prefix", value<std::string>(&prefix), "add prefix to all tasks")
        ("whitelist", value<std::string>(&whiteListInput),"comma-separated list of regular expressions to filter streams")
        ("headless", bool_switch(&headless), "only use the cli")
        ("no-exit", bool_switch(&no_exit), "keep running when replay is finished, only relevant in headless mode")
        ("rename", value<std::vector<std::string>>(&renamingInput), "rename task, e.g. trajectory_follower:traj_follower")
        ("log-files", value<std::vector<std::string>>(&fileArgs), "log files")
        ("quiet", bool_switch(&quiet), "Don't print verbose status updates to stdout");

    positional_options_description p;
    p.add("log-files", -1);

    variables_map vm;
    store(command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
    notify(vm);

    fileNames = LogFileHelper::parseFileNames(fileArgs);
    if(vm.count("help") || !vm.count("log-files") || fileNames.empty())
    {
        std::cout << desc << std::endl;
        return false;
    }

    if(vm.count("whitelist"))
    {
        boost::tokenizer<boost::char_separator<char>> tokens(whiteListInput, boost::char_separator<char>(","));
        whiteListTokens.assign(tokens.begin(), tokens.end());
    }

    if(vm.count("rename"))
    {
        for(const auto& renaming : renamingInput)
        {
            std::vector<std::string> splits;
            boost::split(splits, renaming, boost::is_any_of(":"));

            if(splits.size() == 2)
            {
                renamings.emplace(splits[0], splits[1]);
            }
        }
    }

    return true;
}
