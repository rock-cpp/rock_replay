#pragma once

#include <iostream>
#include <pocolog_cpp/MultiFileIndex.hpp>
#include <base/Time.hpp>
#include <orocos_cpp/PluginHelper.hpp>
#include <rtt/transports/corba/TaskContextServer.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>

#include "LogTask.hpp"

class ReplayHandler
{
  
public:
    ReplayHandler(int argc, char** argv);
    ~ReplayHandler();
    
    void replaySamples();
    
    void toggle();

    
    void next();
    void previous();
    void setSampleIndex(uint index);
    
    void setReplayFactor(double factor);
    double getActualSpeed() const;
    
    inline const std::map<std::string, LogTask*>& getAllLogTasks()
    {
        return logTasks;
    }
    
    
    
private:  
    double replayFactor;
    mutable double actualSpeed;
    uint curIndex;
    bool finished;
    
    bool play;
    boost::thread *replayThread;
    boost::condition_variable cond;
    boost::mutex mut;
    
    std::vector<std::string> filenames;
    std::map<std::string, LogTask *> logTasks;
    std::vector<LogTask *> streamToTask;
    pocolog_cpp::MultiFileIndex *multiIndex;
    
    void replaySample(size_t index) const;
    const base::Time getTimeStamp(size_t globalIndex) const;
};