#include "ArgParser.hpp"
#include "ReplayGui.h"

#include <csignal>

ReplayHandler replayHandler;

void startHeadless(const ArgParser& argParser)
{
    static bool no_exit = argParser.no_exit;
    std::signal(SIGINT, [](int sig) { replayHandler.stop(); no_exit = false; });
    replayHandler.init(argParser.fileNames, argParser.prefix, argParser.whiteListTokens, argParser.renamings);
    replayHandler.play();

    while(replayHandler.isPlaying())
    {
        if(!argParser.quiet){
            std::cout << "replaying [" << replayHandler.getCurIndex() << "/" << replayHandler.getMaxIndex()
                      << "]: " << replayHandler.getCurSamplePortName() << "\r";
        }
        usleep(5000);
    }

    std::cout << std::endl;

    replayHandler.stop();
    std::cout << "replay handler stopped" << std::endl;
    
    while(no_exit) sleep(1);
}

int startGui(int argc, char* argv[], const ArgParser& argParser)
{
    QApplication a(argc, argv);
    ReplayGui gui;

    gui.initReplayHandler(argParser.fileNames, argParser.prefix, argParser.whiteListTokens, argParser.renamings);
    gui.updateTaskView();

    gui.show();
    return a.exec();
}

int main(int argc, char* argv[])
{
    ArgParser argParser;
    if(argParser.parseArguments(argc, argv))
    {
        if(argParser.headless)
        {
            startHeadless(argParser);
        }
        else
        {
            startGui(argc, argv, argParser);
        }
    }

    return 0;
}
