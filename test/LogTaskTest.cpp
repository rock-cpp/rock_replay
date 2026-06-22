#include "LogTask.hpp"

#include "FileLocationHandler.hpp"
#include "LogFileHelper.hpp"

#include <boost/test/unit_test.hpp>
#include <orocos_cpp/orocos_cpp.hpp>
#include <pocolog_cpp/MultiFileIndex.hpp>
#include <rtt/InputPort.hpp>
#include <rtt/base/InputPortInterface.hpp>
#include <thread>
#include <trajectory_follower/TrajectoryFollowerTypes.hpp>

const std::string logFolder = getLogFilePath();
const auto fileNames = LogFileHelper::parseFileNames({logFolder + "trajectory_follower_Logger.0.log"});
pocolog_cpp::MultiFileIndex multiFileIndex = pocolog_cpp::MultiFileIndex(false);
orocos_cpp::OrocosCpp orocos;
std::unique_ptr<LogTask> trajectoryFollowerTask;

void setup()
{
    orocos_cpp::OrocosCppConfig config;
    config.package_initialization_whitelist = {"trajectory_follower"};
    orocos.initialize(config);

    multiFileIndex.registerStreamCheck([&](pocolog_cpp::Stream* st) { return dynamic_cast<pocolog_cpp::InputDataStream*>(st); });
    multiFileIndex.createIndex(fileNames);
}

std::unique_ptr<LogTask> createLogTask(const std::string& taskName, const std::string& prefix = "", const std::string& renaming = "")
{
    try
    {
        orocos.loadAllTypekitsForModel(taskName);
    }
    catch(...)
    {
    }

    std::unique_ptr<LogTask> logTask = std::unique_ptr<LogTask>(new LogTask(taskName, prefix, renaming));
    return logTask;
}

BOOST_AUTO_TEST_CASE(TestTaskCreation)
{
    setup();

    trajectoryFollowerTask = createLogTask("trajectory_follower");
    auto missingTask = createLogTask("non_existing", "prefix/");

    BOOST_TEST(trajectoryFollowerTask->isValid());
    BOOST_TEST(missingTask->isValid());

    BOOST_TEST(trajectoryFollowerTask->getName() == "trajectory_follower");
    BOOST_TEST(missingTask->getName() == "prefix/non_existing");
}

BOOST_AUTO_TEST_CASE(TestRenameTask)
{
    setup();

    auto otherTrajFollower = createLogTask("trajectory_follower", "", "other_trajectory_follower");

    BOOST_TEST(otherTrajFollower->isValid());
    BOOST_TEST(otherTrajFollower->getName() == "other_trajectory_follower");
}

BOOST_AUTO_TEST_CASE(TestStreamLoadingSuccessful)
{
    for(const auto& stream : multiFileIndex.getAllStreams())
    {
        trajectoryFollowerTask->addStream(*dynamic_cast<pocolog_cpp::InputDataStream*>(stream));
    }

    std::vector<std::string> expectedPorts = {"motion_command", "state", "follower_data"};
    for(const auto& port2Type : trajectoryFollowerTask->getPortCollection())
    {
        expectedPorts.erase(std::find(expectedPorts.begin(), expectedPorts.end(), port2Type.first));
    }

    BOOST_TEST(expectedPorts.empty());
}

BOOST_AUTO_TEST_CASE(TestStreamLoadingWithWrongTask)
{
    auto missingTask = createLogTask("non_existing");
    auto inputStream = *dynamic_cast<pocolog_cpp::InputDataStream*>(multiFileIndex.getAllStreams().front());
    bool streamWasAdded = missingTask->addStream(inputStream);

    BOOST_TEST(missingTask->isValid());
    BOOST_TEST(missingTask->getPortCollection().empty());
    BOOST_TEST(!streamWasAdded);
}

BOOST_AUTO_TEST_CASE(TestAddingWrongStream)
{
    auto slamMultiFileIndex = pocolog_cpp::MultiFileIndex(false);
    slamMultiFileIndex.createIndex(LogFileHelper::parseFileNames({logFolder + "slam3d_Logger.0.log"}));

    for(const auto& stream : slamMultiFileIndex.getAllStreams())
    {
        bool streamWasAdded = trajectoryFollowerTask->addStream(*dynamic_cast<pocolog_cpp::InputDataStream*>(stream));

        BOOST_TEST(!streamWasAdded);
        BOOST_TEST(trajectoryFollowerTask->getPortCollection().size() == 3);
    }
}

void replayAllSamplesOfTask(std::unique_ptr<LogTask>& logTask)
{
    for(size_t i = 0; i < multiFileIndex.getSize(); i++)
    {
        pocolog_cpp::InputDataStream* inputStream = dynamic_cast<pocolog_cpp::InputDataStream*>(multiFileIndex.getSampleStream(i));

        if(inputStream->getName().find(logTask->getName()) != std::string::npos)
        {
            logTask->replaySample(inputStream->getIndex(), multiFileIndex.getPosInStream(i));
        }
    }
}

template <typename PortType> RTT::InputPort<PortType>* createPortReader(const std::string& taskName, const std::string& portName)
{
    auto task = orocos.getTaskContext(taskName);
    auto port = task->getPort(portName);
    auto reader = dynamic_cast<RTT::InputPort<PortType>*>(port->antiClone());

    reader->setName(portName + "_reader");
    task->addPort(*reader);
    reader->connectTo(port, RTT::ConnPolicy());

    return reader;
}

BOOST_AUTO_TEST_CASE(TestPortReplay)
{
    auto portReader = createPortReader<base::commands::Motion2D>("trajectory_follower", "motion_command");

    replayAllSamplesOfTask(trajectoryFollowerTask);

    auto sample = portReader->getDataSource();
    BOOST_TEST(portReader->read(sample) == RTT::FlowStatus::NewData);
}

BOOST_AUTO_TEST_CASE(TestPortReplayDeactivated)
{
    auto portReader = createPortReader<base::commands::Motion2D>("trajectory_follower", "motion_command");
    trajectoryFollowerTask->activateLoggingForPort("motion_command", false);

    replayAllSamplesOfTask(trajectoryFollowerTask);

    auto sample = portReader->getDataSource();
    BOOST_TEST(portReader->read(sample) == RTT::FlowStatus::NoData);
}

BOOST_AUTO_TEST_CASE(TestTaskStateChange)
{
    auto initialState = orocos.getTaskContext("trajectory_follower")->getTaskState();
    auto portReader = createPortReader<int>("trajectory_follower", "state");

    replayAllSamplesOfTask(trajectoryFollowerTask);

    BOOST_TEST(portReader->connected());
    BOOST_TEST(initialState != orocos.getTaskContext("trajectory_follower")->getTaskState());
}