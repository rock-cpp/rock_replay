#pragma once

#include "LogTask.hpp"

#include <map>
#include <memory>
#include <orocos_cpp/orocos_cpp.hpp>
#include <pocolog_cpp/MultiFileIndex.hpp>
#include <string>
#include <vector>

/**
 * @brief Class that handles the instantiation of LogTasks and their ports.
 * It takes ownership of all instantiated LogTasks and MultiFileIndex streams and
 * offers a convenient interface, abstracting the MultiFileIndex structure.
 *
 */
class LogTaskManager
{
public:
    /**
     * @brief Structure for currently loaded sample metadata.
     */
    struct SampleMetadata
    {
        /**
         * @brief Name of currently loaded port.
         */
        std::string portName;

        /**
         * @brief Timestamp of currently loaded sample.
         */
        base::Time timeStamp;

        /**
         * @brief Indicates whether loading from MultiFileIndex stream was valid.
         * Does not indicate if unmarshaling can be performed.
         */
        bool valid;
    };

    /**
     * @brief Pair of port name and corresponding c++ data type.
     */
    using PortInfo = std::pair<std::string, std::string>;

    /**
     * @brief List of PortInfos.
     */
    using PortCollection = std::vector<PortInfo>;

    /**
     * @brief Map of task name to corresponding list of PortInfos.
     */
    using TaskCollection = std::map<std::string, LogTask::PortCollection>;

    /**
     * @brief Constructor.
     */
    LogTaskManager();

    /**
     * @brief Destructor.
     */
    ~LogTaskManager() = default;

    /**
     * @brief Initializes the LogTaskManager with new LogTasks and a optionally with a prefix.
     *
     * @param fileNames: List of filenames to load. Filenames must be absolute.
     * @param prefix: Prefix to add for all LogTasks.
     * @param whiteList: List of regular expressions to filter whitelisted streams.
     * @param renamings: Map of task renamings.
     */
    void init(
        const std::vector<std::string>& fileNames, const std::string& prefix, const std::vector<std::string>& whiteList = {},
        const std::map<std::string, std::string>& renamings = {});

    /**
     * @brief Sets the replay pointer to the given index. The sample
     * is not actually replayed after this method is called.
     *
     * @param index: Index to set. Index is clamped if min/max span is set.
     * @return LogTaskManager::SampleMetadata Struct containing metadata of current sample.
     */
    SampleMetadata setIndex(size_t index);

    /**
     * @brief Replays the currently set sample.
     *
     * @return bool True if the sample was replayed successfully, false otherwise (e.g. on marshal error).
     */
    bool replaySample();

    /**
     * @brief Enables or disabled replaying for a given port of a task.
     *
     * @param taskName: Name of task.
     * @param portName: Name of port.
     * @param on: True if replaying should be enabled, false otherwise.
     */
    void activateReplayForPort(const std::string& taskName, const std::string& portName, bool on);

    /**
     * @brief Returns a map of task names to a list of their ports with types.
     *
     * @return LogTaskManager::TaskCollection Structure containing task names with list of ports and types.
     */
    TaskCollection getTaskCollection();

    /**
     * @brief Returns the number of samples found in the logfiles.
     *
     * @return size_t Number of samples.
     */
    size_t getNumSamples();

private:
    /**
     * @brief Searches for a LogTask given the stream name. If no LogTask
     * was instantiated for the stream, a new one is created and inserted.
     *
     * @param streamName: Name of the stream as defined in the MultiFileIndex.
     * @return LogTask& Reference to the corresponding LogTask for the stream.
     */
    LogTask& findOrCreateLogTask(const std::string& streamName);

    /**
     * @brief Loads the typekit for the given stream.
     * If the stream does not contain a model name in its metadata, a loading
     * via the stream name is attempted.
     * Finally, the stream is added to its corresponding log task.
     *
     * @param inputStram: InputDataStream to load typekits for and to add to a log task.
     * @return True if the stream was added to the log task.
     */
    bool loadTypekitsAndAddStreamToLogTask(pocolog_cpp::InputDataStream& inputStream);

    /**
     * @brief Prefix for all LogTasks.
     *
     */
    std::string prefix;

    /**
     * @brief MultiFileIndex containing all datastreams from logfiles.
     * This class has the ownership. If the MultiFileIndex is cleared, all
     * LogTasks must be deleted beforehand as they become invalid.
     *
     */
    pocolog_cpp::MultiFileIndex multiFileIndex;

    /**
     * @brief Orocos api object. Gets forwarded to LogTasks to instantiate the Orocos tasks.
     *
     */
    orocos_cpp::OrocosCpp orocos;

    /**
     * @brief Function to call when the previously selected sample should be replayed.
     * Used to decouple setting of index (and thus getting metadata information) and actual replay.
     *
     */
    std::function<bool()> replayCallback;

    /**
     * @brief Container that maps stream names (as trajectory_follower.motion_command) to their correspoding LogTask instances.
     *
     */
    std::map<std::string, std::shared_ptr<LogTask>> streamName2LogTask;

    /**
     * @brief Map of renamings for tasks.
     * 
     */
    std::map<std::string, std::string> renamings;
};
