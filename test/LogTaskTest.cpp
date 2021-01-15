#include "LogTask.hpp"

#include "LogFileHelper.hpp"

#include <boost/test/unit_test.hpp>
#include <orocos_cpp/orocos_cpp.hpp>
#include <pocolog_cpp/MultiFileIndex.hpp>
#include <rtt/InputPort.hpp>
#include <rtt/base/InputPortInterface.hpp>
#include <thread>
#include <trajectory_follower/TrajectoryFollowerTypes.hpp>

const auto fileNames = LogFileHelper::parseFileNames({"../logs/trajectory_follower_Logger.0.log"});
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

std::unique_ptr<LogTask> createLogTask(const std::string& taskName, const std::string& prefix = "")
{
    std::unique_ptr<LogTask> logTask = std::unique_ptr<LogTask>(new LogTask(taskName, prefix, orocos));
    return logTask;
}

BOOST_AUTO_TEST_CASE(TestTaskCreation)
{
    setup();

    trajectoryFollowerTask = createLogTask("trajectory_follower");
    auto missingTask = createLogTask("non_existing", "prefix/");

    BOOST_TEST(trajectoryFollowerTask->isValid());
    BOOST_TEST(!missingTask->isValid());

    BOOST_TEST(trajectoryFollowerTask->getName() == "trajectory_follower");
    BOOST_TEST(missingTask->getName() == "prefix/non_existing");
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

// BOOST_AUTO_TEST_CASE(TestStreamLoadingWithWrongStream)
// {
//     pocolog_cpp::MultiFileIndex slamMultiFile = pocolog_cpp::MultiFileIndex(false);
//     slamMultiFile.registerStreamCheck([&](pocolog_cpp::Stream* st) { return dynamic_cast<pocolog_cpp::InputDataStream*>(st); });
//     slamMultiFile.createIndex(LogFileHelper::parseFileNames({"./logs/contact_point_odometry_Logger.0.log"}));

//     BOOST_TEST(slamMultiFile.getAllStreams().size() == 1);
// }

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
    trajectoryFollowerTask->activateLoggingForPort("motion_command");

    std::thread t = std::thread([&]() { replayAllSamplesOfTask(trajectoryFollowerTask); });

    auto sample = portReader->getDataSource();
    while(portReader->read(sample) != RTT::FlowStatus::NewData)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        sample = portReader->getDataSource();
    }

    t.join();
}