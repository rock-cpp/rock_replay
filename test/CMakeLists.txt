rock_testsuite(
    test_suite
        ArgParserTest.cpp
        Main.cpp
        LogFileHelperTest.cpp
        LogTaskManagerTest.cpp
        LogTaskTest.cpp
        ReplayHandlerTest.cpp
        WhiteListTest.cpp
    DEPS 
        rock_replay
        arg_parser
    DEPS_PKGCONFIG
        trajectory_follower
        orocos_cpp
)

target_include_directories(test_suite PRIVATE "../src")
