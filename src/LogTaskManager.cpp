#include "LogTaskManager.hpp"

#include "LogFileHelper.hpp"

#include <base-logging/Logging.hpp>
#include <orocos_cpp/orocos_cpp.hpp>

LogTaskManager::LogTaskManager()
{
    orocos_cpp::OrocosCppConfig config;
    orocos.initialize(config);
}

void LogTaskManager::init(
    const std::vector<std::string>& fileNames, const std::string& prefix, const std::vector<std::string>& whiteList,
    const std::map<std::string, std::string>& renamings)
{
    this->prefix = prefix;
    this->renamings = renamings;
    
    streamName2LogTask.clear();
    multiFileIndex = pocolog_cpp::MultiFileIndex(false);
    multiFileIndex.registerStreamCheck([&](pocolog_cpp::Stream* st) {
        LOG_INFO_S << "Checking " << st->getName();

        if(LogFileHelper::isWhiteListed(st->getName(), whiteList))
        {
            pocolog_cpp::InputDataStream* inputSt = dynamic_cast<pocolog_cpp::InputDataStream*>(st);
            return loadTypekitsAndAddStreamToLogTask(*inputSt);
        }
        else
        {
            LOG_INFO_S << "Skipping non-whitelisted stream " << st->getName();
        }

        return false;
    });

    multiFileIndex.createIndex(fileNames);
}

LogTaskManager::SampleMetadata LogTaskManager::setIndex(size_t index)
{
    try
    {
        pocolog_cpp::InputDataStream* inputStream = dynamic_cast<pocolog_cpp::InputDataStream*>(multiFileIndex.getSampleStream(index));
        replayCallback = [=]() {
            return streamName2LogTask.at(inputStream->getName())->replaySample(inputStream->getIndex(), multiFileIndex.getPosInStream(index));
        };

        return {inputStream->getName(), inputStream->getFileIndex().getSampleTime(multiFileIndex.getPosInStream(index)), true};
    }
    catch(...)
    {
    }

    return {"", base::Time(), false};
}

bool LogTaskManager::replaySample()
{
    try
    {
        return replayCallback(); // TODO: give replay feedback and reset und stop. Maybe differentiate between deactivated ports and ports with no
                                 // handles.
    }
    catch(...)
    {
        return false;
    }

    return true;
}

LogTaskManager::TaskCollection LogTaskManager::getTaskCollection()
{
    TaskCollection taskNames2PortInfos;
    for(const auto& streamNameTaskPair : streamName2LogTask)
    {
        const auto& task = streamNameTaskPair.second;
        taskNames2PortInfos.emplace(task->getName(), task->getPortCollection());
    }

    return taskNames2PortInfos;
}

size_t LogTaskManager::getNumSamples()
{
    return multiFileIndex.getSize();
}

LogTask& LogTaskManager::findOrCreateLogTask(const std::string& streamName)
{
    std::shared_ptr<LogTask> logTask;
    const auto taskNameAndPort = LogFileHelper::splitStreamName(streamName);

    for(const auto& name2LogTask : streamName2LogTask)
    {
        if(LogFileHelper::splitStreamName(name2LogTask.first).first == taskNameAndPort.first)
        {
            logTask = name2LogTask.second;
        }
    }

    if(!logTask)
    {
        std::string renaming;
        if(renamings.find(taskNameAndPort.first) != renamings.end())
        {
            renaming = renamings.at(taskNameAndPort.first);
        }

        logTask = std::make_shared<LogTask>(taskNameAndPort.first, prefix, renaming);
    }

    streamName2LogTask.emplace(streamName, logTask);

    return *logTask;
}

bool LogTaskManager::loadTypekitsAndAddStreamToLogTask(pocolog_cpp::InputDataStream& inputStream)
{
    std::string modelName = LogFileHelper::splitStreamName(inputStream.getName()).first;

    try
    {
        modelName = inputStream.getTaskModel();
    }
    catch(...)
    {
        LOG_WARN_S << "stream " << inputStream.getName()
                   << " does not contain necessary metadata for its task model. Trying to load typekit via stream name";
    }

    try
    {
        orocos.loadAllTypekitsForModel(modelName);
        LogTask& logTask = findOrCreateLogTask(inputStream.getName());
        return logTask.addStream(inputStream);
    }
    catch(...)
    {
        LOG_WARN_S << "cannot load typekits for stream " << inputStream.getName();
    }

    return false;
}

void LogTaskManager::activateReplayForPort(const std::string& taskName, const std::string& portName, bool on)
{
    std::string taskNameWithPossiblePrefix = taskName;
    std::string::size_type index = taskNameWithPossiblePrefix.find(prefix);

    if(index != std::string::npos)
    {
        taskNameWithPossiblePrefix.erase(index, prefix.length());
    }

    LogTask& logTask = findOrCreateLogTask(taskNameWithPossiblePrefix);
    logTask.activateLoggingForPort(portName, on);
}
