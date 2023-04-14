cmake_minimum_required(VERSION 3.15)
project(TFliteWrapper C CXX)
set(LIB_NAME timbreid)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# set(CMAKE_CXX_FLAGS "-Wall -Wextra")
# set(CMAKE_CXX_FLAGS_DEBUG "-g")
# set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# add_subdirectory(
#   "${tensorflow_SOURCE_DIR}/tensorflow/lite"
#   "${CMAKE_CURRENT_BINARY_DIR}/tensorflow-lite"
#   EXCLUDE_FROM_ALL
# )

# Uncomment for debug purposes
# message(WARNING "tensorflow_SOURCE_DIR: ${tensorflow_SOURCE_DIR}")
# message(WARNING "subdir1: ${tensorflow_SOURCE_DIR}/tensorflow/lite")
# message(WARNING "subdir2: ${CMAKE_CURRENT_BINARY_DIR}/tensorflow-lite")

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(CMAKE_CXX_STANDARD 11)

include_directories(test PRIVATE "include/")
include_directories(test PRIVATE "libs/")


add_library(${LIB_NAME}
    src/attackTime.cpp
    # src/barkSpecBrightness.cpp
    # src/barkSpec.cpp
    # src/bfcc.cpp
    # src/bin2freq.cpp
    # src/cepstrum.cpp
    # src/freq2bin.cpp
    # src/mfcc.cpp
    # src/peakSample.cpp
    # src/tIDLib.cpp
    # src/zeroCrossing.cpp
)
target_link_libraries(${LIB_NAME}
    fftw3f
)



# add_executable(libtest
#     tests/main.cpp
# )
# target_link_libraries(libtest
# ${LIB_NAME}
# )

# add_executable(tflite_test_bulk
#     src/test/test2.cpp
# )
# target_link_libraries(tflite_test_bulk
#     tflitewrapper
# )






# Testing

# enable_testing()

add_executable(base_tests
    tests/base_tests.cpp
)
target_link_libraries(base_tests
    gtest_main
)
target_link_libraries(base_tests
    ${LIB_NAME}
)

include(GoogleTest)
gtest_discover_tests(base_tests)

# add_test( hello_test hello_test )
set(UNIT_TEST base_tests)
add_test(NAME    ${UNIT_TEST}
         COMMAND base_tests)

add_custom_command(
     TARGET ${UNIT_TEST}
     COMMENT "Run tests"
     POST_BUILD 
     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
     COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION> -R "^${UNIT_TEST}$" --output-on-failures
)