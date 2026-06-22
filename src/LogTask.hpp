#pragma once

#include <orocos_cpp/orocos_cpp.hpp>
#include <pocolog_cpp/InputDataStream.hpp>
#include <rtt/TaskContext.hpp>
#include <rtt/base/OutputPortInterface.hpp>
#include <rtt/transports/corba/CorbaDispatcher.hpp>
#include <rtt/transports/corba/TaskContextServer.hpp>
#include <rtt/typelib/TypelibMarshallerBase.hpp>
#include <rtt/types/Types.hpp>
#include <string>

/**
 * @brief Class that represents a LogTask instance. It contains a pointer to a Orocos task
 * and offers some conevient api functions.
 *
 */
class LogTask
{
    /**
     * @brief Struct that contains all port-related orocos data types.
     *
     */
    struct PortHandle
    {
        /**
         * @brief Constructor.
         *
         * @param name: Name of the port.
         * @param transport: Orogen tansport of port.
         * @param sample: Sample to contain unmarshaled data.
         * @param transportHandle: Orogen transport handle.
         * @param port: Orocos port.
         * @param active: True if port is enabled, false otherwise.
         * @param inputDataStream: Related InputDataStream from Logfile.
         */
        PortHandle(
            const std::string& name, orogen_transports::TypelibMarshallerBase* transport, RTT::base::DataSourceBase::shared_ptr sample,
            orogen_transports::TypelibMarshallerBase::Handle* transportHandle, RTT::base::OutputPortInterface* port, bool active,
            pocolog_cpp::InputDataStream& inputDataStream)
            : name(name)
            , transport(transport)
            , sample(sample)
            , transportHandle(transportHandle)
            , port(port)
            , active(active)
            , inputDataStream(inputDataStream)
        {
        }

        /**
         * @brief Name of port.
         *
         */
        std::string name;

        /**
         * @brief Orogen tansport of port.
         *
         */
        orogen_transports::TypelibMarshallerBase* transport;

        /**
         * @brief Sample to contain unmarshaled data.
         *
         */
        RTT::base::DataSourceBase::shared_ptr sample;

        /**
         * @brief Orogen transport handle.
         *
         */
        orogen_transports::TypelibMarshallerBase::Handle* transportHandle;

        /**
         * @brief Orocos port.
         *
         */
        RTT::base::OutputPortInterface* port;

        /**
         * @brief Indicates whether the port is active or not.
         *
         */
        bool active;

        /**
         * @brief Related InputDataStream from Logfile.
         *
         */
        pocolog_cpp::InputDataStream& inputDataStream;
    };

public:
    /**
     * @brief Alias for pair of port name and port type.
     *
     */
    using PortInfo = std::pair<std::string, std::string>;

    /**
     * @brief Alias for list of PortInfo.
     *
     */
    using PortCollection = std::vector<PortInfo>;

    /**
     * @brief Constructor.
     *
     * @param taskName: Name of orocos task.
     * @param prefix: Prefix to add for task.
     * @param renaming: Renaming for task.
     */
    LogTask(const std::string& taskName, const std::string& prefix, const std::string& renaming = "");

    /**
     * @brief Destructor.
     * Removes the orocos task from the corba server. The task is deleted automatically.
     */
    ~LogTask();

    /**
     * @brief Adds a stream from the logfile to the task.
     * The method can return false if either no typekit was found for the
     * stream, or the stream does not contain data for the task model (name-based check).
     *
     * @param stream: InputDataStream from logfile containing port samples.
     * @return bool True if stream was added, false otherwise.
     */
    bool addStream(pocolog_cpp::InputDataStream& stream);

    /**
     * @brief Replays a given sample by global stream index and position in that stream.
     *
     * @param streamIndex: Global stream index from logfile.
     * @param indexInStream: Sample position in that stream.
     */
    bool replaySample(uint64_t streamIndex, uint64_t indexInStream);

    /**
     * @brief Activates logging for that port.
     * If disabled, replaying does nothing for the port.
     *
     * @param portName: Name of the port.
     * @param activate: True if replaying should unmarshal and replay the data, false otherwise.
     */
    void activateLoggingForPort(const std::string& portName, bool activate = true);

    /**
     * @brief Returns the PortCollection of all ports this task owns.
     *
     * @return PortCollection List of pairs of ports and their types.
     */
    PortCollection getPortCollection();

    /**
     * @brief Returns the task's name.
     *
     * @return std::string Name of the task.
     */
    std::string getName();

    /**
     * @brief Returns if the LogTask is valid, e.g. a typekit could be loaded/found for the task
     *
     * @return bool True if the LogTask was successfully initialized, false otherwise.
     */
    bool isValid();

private:
    /**
     * @brief Creates a PortHandle given the port name and corresponding InputDataStream from logfile.
     *
     * @param portName: Name of the port.
     * @param inputStream: InputDataStream from logfile for the port.
     * @return std::unique_ptr<LogTask::PortHandle> Unique pointer with PortHandle. Caller obtains ownership.
     */
    std::unique_ptr<PortHandle> createPortHandle(const std::string& portName, pocolog_cpp::InputDataStream& inputStream);

    /**
     * @brief Returns whether replay of a port can be skipped.
     *
     * @param result: True if skipping was caused by an unconnected port, false is replaying was disabled or port is invalid.
     * @param portHandle: Handle to check.
     * @return bool True if port can be skipped because there is no need to replay, false otherwise.
     */
    bool canPortBeSkipped(bool& result, std::unique_ptr<PortHandle>& portHandle);
    /**
     * @brief Unmarshals a given sample from the corresponding InputDataStream.
     * An unmarshaled sample is then hold in the handle's sample pointer.
     *
     * @param portHandle: Port to use for unmarshaling.
     * @param indexInStream: Position of sample to unmarshal in InputDataStream.
     * @return bool True if unmarshaling was performed successfully, false otherwise.
     */

    bool unmarshalSample(std::unique_ptr<PortHandle>& portHandle, uint64_t indexInStream);

    /**
     * @brief Checks whether the given PortHandle is a state port and applies a task state change.
     *
     * @param portHandle: PortHandle to check.
     */
    void checkTaskStateChange(std::unique_ptr<PortHandle>& portHandle);

    /**
     * @brief Checks is the given stream is suitable for the task model.
     * Check is based on names.
     *
     * @param inputStream: InputDataStream to check.
     * @return bool True if the stream is suitable for the task, false otherwise.
     */
    bool isStreamForThisTask(const pocolog_cpp::InputDataStream& inputStream);

    /**
     * @brief Contains the possibly prefixed name of the task.
     *
     */
    std::string prefixedName;

    /**
     * @brief Original task name in case of renaming.
     * 
     */
    std::string originalName;

    /**
     * @brief Orocos task context. Is deleted automatically on LogTask destructor call.
     *
     */
    std::unique_ptr<RTT::TaskContext> task;

    /**
     * @brief Map of global stream indices to corresponding port handles.
     * Is deleted automatically on LogTask destructor call.
     *
     */
    std::map<uint64_t, std::unique_ptr<PortHandle>> streamIdx2Port;
};
