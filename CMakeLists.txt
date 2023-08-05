cmake_minimum_required(VERSION 3.21)
project(mipworkshop2024)

set(CMAKE_CXX_STANDARD 20)
option(MIPWORKSHOP2024_BUILD_TESTS "Turn on to compile the tests" ON)

find_package(Boost COMPONENTS iostreams REQUIRED)
add_library(mipworkshop2024
        src/Problem.cpp
        src/Shared.cpp
        src/Solution.cpp
        src/IO.cpp
        src/SparseMatrix.cpp
        )
target_include_directories(mipworkshop2024
        PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        )
target_link_libraries(mipworkshop2024
        PUBLIC Boost::iostreams
        )

add_subdirectory(apps)

if(MIPWORKSHOP2024_BUILD_TESTS)
    add_subdirectory(tests)
endif()