#pragma once

#include <vector>
#include <string>
#include <memory>
#include <map>

#include <orocos_cpp/orocos_cpp.hpp>
#include <pocolog_cpp/MultiFileIndex.hpp>

#include "LogTask.hpp"

class LogTaskManager
{  
public:
    struct SampleMetadata 
    {
        std::string portName;
        base::Time timeStamp;
        bool valid;
    };
    
    struct GlobalIndexInfo
    {
        std::string taskName;
        bool canReplay;
    };
    
    LogTaskManager();
    ~LogTaskManager() = default;
    
    void init(const std::vector<std::string>& fileNames);
    void deinit();
    SampleMetadata setIndex(size_t index);
    bool replaySample();
    void activateReplayForPort(const std::string& taskName, const std::string& portName, bool on);
    const std::map<std::string, LogTask>& getAllLogTasks();
    size_t getNumSamples();
    
private:
    std::string getTaskName(pocolog_cpp::InputDataStream* stream);
    void loadTypekits(const std::set<std::string>& modelsToLoad);
    void createLogTasks();
    bool addStreamToTask(std::map<std::string, LogTask>::iterator& taskIt, const std::string& taskName, const pocolog_cpp::InputDataStream& inputStream);
    
    pocolog_cpp::MultiFileIndex multiFileIndex;
    std::map<std::string, LogTask> name2Task;
    std::map<size_t, GlobalIndexInfo> globalIndex2TaskName;
    orocos_cpp::OrocosCpp orocos;
    std::function<bool()> replayCallback;
};
