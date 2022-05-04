cmake_minimum_required(VERSION 3.22)
project(processes_transform)

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER g++-10)

include_directories(utils)

set(
        transform_src
        ${processes_transform_SOURCE_DIR}/utils/Transform.h
        ${processes_transform_SOURCE_DIR}/utils/SharedArray.h
)

add_executable(main
        ${transform_src}
        main.cpp)
target_link_libraries(main pthread)

add_subdirectory(tests)