# CMakeLists.txt has to be located in the project folder and cmake has to be
# executed from 'project/build' with 'cmake ../'.
cmake_minimum_required(VERSION 3.9)
project(rock_replay
        VERSION 0.2
        DESCRIPTION "application to replay log files via tasks")
find_package(Rock)

add_compile_options(--coverage -Wall)

rock_init()
rock_feature(NOCURDIR)
rock_standard_layout()
