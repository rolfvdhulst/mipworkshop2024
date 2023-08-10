cmake_minimum_required(VERSION 3.21)
project(mipworkshop2024)

set(CMAKE_CXX_STANDARD 20)
option(MIPWORKSHOP2024_BUILD_TESTS "Turn on to compile the tests" ON)

find_package(Boost COMPONENTS iostreams REQUIRED)
find_package(SCIP REQUIRED)

add_library(mipworkshop2024
        src/Problem.cpp
        src/Shared.cpp
        src/ExternalSolution.cpp
        src/IO.cpp
        src/SparseMatrix.cpp
        src/ApplicationShared.cpp
        src/Solution.cpp
        src/presolve/PostSolveStack.cpp
        src/presolve/PresolvingProblem.cpp
        src/presolve/IncidenceAddition.cpp
        src/Submatrix.cpp
        src/Solve.cpp
        src/Logging.cpp
        src/presolve/Presolver.cpp
        )
target_include_directories(mipworkshop2024
        PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"

        )
target_link_libraries(mipworkshop2024
        PUBLIC Boost::iostreams
        PUBLIC ${SCIP_LIBRARIES}
        )

add_subdirectory(apps)

if(MIPWORKSHOP2024_BUILD_TESTS)
    add_subdirectory(tests)
endif()