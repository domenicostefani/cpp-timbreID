cmake_minimum_required(VERSION 2.6)
project(UnitTestsPostOnsetTimer)

# Locate GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Link runTests with what we want tp test and the Gtest and pthread library
add_executable(executeTests tests.cpp)
target_link_libraries(executeTests ${GTEST_LIBRARIES} pthread)
