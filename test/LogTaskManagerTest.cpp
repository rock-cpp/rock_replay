#include "LogTaskManager.hpp"

#include "FileLocationHandler.hpp"
#include "LogFileHelper.hpp"

#include <boost/test/unit_test.hpp>
#include <set>

const std::string logFolder = getLogFilePath();
const auto fileNames = LogFileHelper::parseFileNames({logFolder + "trajectory_follower_Logger.0.log"});
LogTaskManager manager;

BOOST_AUTO_TEST_CASE(TestEmptyLoading)
{
    manager.init({}, "");
    BOOST_TEST(!manager.getNumSamples());
}

BOOST_AUTO_TEST_CASE(TestSpawnTrajectoryFollower)
{
    const std::set<std::pair<std::string, std::string>> portsWithTypes = {
        {"follower_data", "/trajectory_follower/FollowerData"}, {"motion_command", "/base/commands/Motion2D"}, {"state", "int"}};

    manager.init(fileNames, "");

    auto tasks = manager.getTaskCollection();
    auto trajectoryFollowerTask = tasks.at("trajectory_follower");
    auto orderedPortsWithType = std::set<std::pair<std::string, std::string>>(trajectoryFollowerTask.begin(), trajectoryFollowerTask.end());

    BOOST_TEST(manager.getNumSamples() == 849);
    BOOST_TEST(tasks.size() == 1);
    BOOST_TEST(orderedPortsWithType == portsWithTypes);
}

BOOST_AUTO_TEST_CASE(TestTaskMetaData)
{
    auto metadata = manager.setIndex(250);

    BOOST_TEST(metadata.valid);
    BOOST_TEST(metadata.portName == "trajectory_follower.motion_command");
    BOOST_TEST(!metadata.timeStamp.isNull());
}

BOOST_AUTO_TEST_CASE(TestInvalidTaskMetaData)
{
    auto metadata = manager.setIndex(1000);

    BOOST_TEST(!metadata.valid);
    BOOST_TEST(metadata.portName == "");
    BOOST_TEST(metadata.timeStamp.isNull());
}

BOOST_AUTO_TEST_CASE(TestTaskActivateReplayForPort)
{
    manager.setIndex(250);
    bool replayedSampleActivated = manager.replaySample();

    manager.activateReplayForPort("trajectory_follower", "motion_command", false);
    manager.setIndex(250);
    bool replayedSampleDeactivated = manager.replaySample();

    BOOST_TEST(replayedSampleActivated);
    BOOST_TEST(!replayedSampleDeactivated);
}

BOOST_AUTO_TEST_CASE(TestTaskWhitelist)
{
    manager.init(fileNames, "", {"foo"});

    BOOST_TEST(manager.getTaskCollection().empty());
}

BOOST_AUTO_TEST_CASE(TestTaskRenaming)
{
    manager.init(fileNames, "", {}, {{"trajectory_follower", "foo"}});

    auto taskCollection = manager.getTaskCollection();
    BOOST_TEST(taskCollection.size() == 1);

    bool containsRenamedTask = taskCollection.find("foo") != taskCollection.end();
    BOOST_TEST(containsRenamedTask);
}