#pragma once

#include "LogTaskManager.hpp"

#include <base/Time.hpp>
#include <future>
#include <memory>
#include <thread>

/**
 * @brief Class for handling replay of log samples.
 *
 */
class ReplayHandler
{

public:
    /**
     * @brief Constructor.
     *
     */
    ReplayHandler() = default;

    /**
     * @brief Destructor.
     *
     */
    ~ReplayHandler();

    /**
     * @brief Stops replay. The current index is reset either to 0 or the min span.
     */
    void stop();

    /**
     * @brief Starts replay. Execution is stopped after thre last sample was played.
     *
     */
    void play();

    /**
     * @brief Pauses replay. Execution can be resumed from the current index.
     *
     */
    void pause();

    /**
     * @brief Moves the current index to the next sample. Only updates the metadata, actual replay
     * of sample is not performed. Can only be used when handler is paused/stopped.
     */
    void next();

    /**
     * @brief Moves the current index to the previous sample. Only updates the metadata, actual replay
     * of sample is not performed. Can only be used when handler is paused/stopped.
     */
    void previous();

    /**
     * @brief Moves the current index to the given position. Can only be used it
     * the handler is paused/stopped.
     *
     * @param index: Index to set. Must be in [0, maxIndex].
     */
    void setSampleIndex(uint64_t index);

    /**
     * @brief Sets the replay speed relatively.
     *
     * @param speed: Speed to set. 1.00 means 100%.
     */
    void setReplaySpeed(float speed);

    /**
     * @brief Sets the minimum span. If replay has finished or is stopped, the current
     * index is reset to the minimum span value.
     *
     * @param minIdx: Minimum index.
     */
    void setMinSpan(uint64_t minIdx);

    /**
     * @brief Sets the maximum span. Repaly finishes after the maximum span is reached.
     *
     * @param maxIdx: Maximum index.
     */
    void setMaxSpan(uint64_t maxIdx);

    /**
     * @brief Activates/Deactivates replay for given port of task.
     *
     * @param taskName: Name of task.
     * @param portName: Name of port.
     * @param on: True if replaying should be enabled, false otherwise.
     */
    void activateReplayForPort(const std::string& taskName, const std::string& portName, bool on);

    /**
     * @brief Inits the replay handler for given logfiles.
     * All replay parameters are updated and the current index is set to 0.
     *
     * @param fileNames: List of file names.
     * @param prefix: Prefix for all tasks.
     * @param whiteList: List of regular expressions to filter whitelisted streams.
     * @param renamings: Map of task renamings.
     */
    void init(
        const std::vector<std::string>& fileNames, const std::string& prefix, const std::vector<std::string>& whiteList = {},
        const std::map<std::string, std::string>& renamings = {});

    /**
     * @brief Deinits the replay handler. Closes all log tasks and allows
     * new initing.
     */
    void deinit();

    /**
     * @brief Returns a map of task names with a list of their ports.
     * @return std::map<std::string, std::vector<std::string>> Map of task names with list of ports.
     */
    std::map<std::string, std::vector<std::pair<std::string, std::string>>> getTaskNamesWithPorts();

    /**
     * @brief Returns the current sample timestamp.
     *
     * @return std::string Current timestamp.
     */
    std::string getCurTimeStamp()
    {
        return curMetadata.timeStamp.toString();
    };

    /**
     * @brief Returns the current sample port name.
     *
     * @return std::string Current port name.
     */
    std::string getCurSamplePortName()
    {
        return curMetadata.portName;
    };

    /**
     * @brief Returns the current intex.
     *
     * @return uint Current index.
     */
    uint getCurIndex()
    {
        return curIndex;
    };

    /**
     * @brief Returns the maximum possible index.
     *
     * @return size_t Maximum index.
     */
    size_t getMaxIndex()
    {
        return maxIdx;
    };

    /**
     * @brief Returns the minimum span index.
     *
     * @return uint64_t Minimum span.
     */
    uint64_t getMinSpan()
    {
        return minSpan;
    };

    /**
     * @brief Returns the maximum span index.
     *
     * @return uint64_t Maximum span.
     */
    uint64_t getMaxSpan()
    {
        return maxSpan;
    };

    /**
     * @brief Returns the set relative target speed.
     *
     * @return double Commanded speed.
     */
    double getReplayFactor()
    {
        return targetSpeed;
    };

    /**
     * @brief Returns the reached speed taking in account the target speed.
     *
     * @return double Current speed.
     */
    double getCurrentSpeed()
    {
        return currentSpeed;
    };

    /**
     * @brief Returns whether the current displayed port can be replayed,
     * i.e. if a typelib entry can unmarshal the data.
     *
     * @return bool True if sample can be replayed, false otherwise.
     */
    bool canSampleBeReplayed()
    {
        return curMetadata.valid && replayWasValid;
    };

    /**
     * @brief Returns whether playing has finished.
     *
     * @return bool True if playing is finished, false otherwise.
     */
    bool hasFinished()
    {
        return finished;
    };

    /**
     * @brief Returns whether the replay handler is currently in playing mode.
     *
     * @return bool True if playing is active, false otherwise.
     */
    bool isPlaying()
    {
        return playing;
    };

private:
    /**
     * @brief Starts the replay loop. Must be used in separate replay thread.
     *
     */
    void replaySamples();

    /**
     * @brief Calculates the time to sleep by using the timestamp of the next sample.
     *
     */
    void calculateTimeToSleep();

    /**
     * @brief Calculates the reached speed taking into account the target speed.
     *
     */
    void calculateRelativeSpeed();

    /**
     * @brief Sets the timestamps at the beginning of a playing-step.
     */
    void setTimeStampBaselines();

    /**
     * @brief Target speed.
     *
     */
    float targetSpeed;

    /**
     * @brief Current speed.
     *
     */
    float currentSpeed;

    /**
     * @brief Current sample metadata. Metadata that is displayed is replayed next.
     *
     */
    LogTaskManager::SampleMetadata curMetadata;

    /**
     * @brief Current index.
     *
     */
    uint64_t curIndex;

    /**
     * @brief Minimum span index.
     *
     */
    uint64_t minSpan;

    /**
     * @brief Maximum span index.
     *
     */
    uint64_t maxSpan;

    /**
     * @brief Maximum possible index.
     *
     */
    uint64_t maxIdx;

    /**
     * @brief Indicator if replaying is finished.
     *
     */
    bool finished;

    /**
     * @brief Indicator if replay thread should run.
     *
     */
    bool running;

    /**
     * @brief Indicator if replaying is active.
     *
     */
    bool playing;

    /**
     * @brief Indicator if the multi file index contains samples to play.
     * This cannot be the case, e.g. when no suitable typekits are available
     * or the streams are emtpy.
     */
    bool gotSamplesToPlay;

    /**
     * @brief Mutex to lock replay variables related to replay.
     *
     */
    std::mutex playMutex;

    /**
     * @brief Condition to wake up thread if replay starts or stops.
     *
     */
    std::condition_variable playCondition;

    /**
     * @brief Replay thread.
     *
     */
    std::thread replayThread;

    /**
     * @brief Timestamp of previous sample.
     *
     */
    base::Time previousSampleTime;

    /**
     * @brief Tracks the system time before going to sleep; used to calculate reached speed.
     *
     */
    base::Time timeBeforeSleep;

    /**
     * @brief Time to sleep before replaying next sample.
     *
     */
    int64_t timeToSleep;

    /**
     * @brief Indicates whether the current sample could be replayed.
     */
    bool replayWasValid;

    /**
     * @brief Log task manager.
     *
     */
    LogTaskManager manager;
};
