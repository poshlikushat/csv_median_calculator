cmake_minimum_required(VERSION 3.23)

project(csv_median_calculator LANGUAGES CXX)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.13.0
)

FetchContent_Declare(
        tomlplusplus
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG v3.4.0
)

set(BOOST_ENABLE_CMAKE ON)
set(BOOST_INCLUDE_LIBRARIES program_options accumulators algorithm)

FetchContent_Declare(
        Boost
        URL https://github.com/boostorg/boost/releases/download/boost-1.84.0/boost-1.84.0.tar.xz
)

FetchContent_MakeAvailable(spdlog tomlplusplus Boost)

include(FetchContent)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
)

FetchContent_MakeAvailable(googletest)

add_executable(parser_tests
        tests/test_parser_csv.cpp
        src/parser_csv.cpp
        tests/test_directory_scanner.cpp
        src/directory_scanner.cpp
        src/median_calculator.cpp
        src/median_calculator.hpp
)

target_link_libraries(parser_tests PRIVATE
        GTest::gtest_main
        spdlog::spdlog
        Boost::program_options
        Boost::algorithm
)

include(CTest)
include(GoogleTest)
gtest_discover_tests(parser_tests)

add_executable(${PROJECT_NAME}
        src/main.cpp
        src/parser_csv.cpp
        src/directory_scanner.cpp
        src/median_calculator.cpp
        src/median_calculator.hpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        spdlog::spdlog
        tomlplusplus::tomlplusplus
        Boost::program_options
        Boost::accumulators
        Boost::algorithm
)