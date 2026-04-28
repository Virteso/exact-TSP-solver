cmake_minimum_required(VERSION 3.10)
project(TSP_Solver)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")

# Add cmake directory to module path
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Check for CPLEX
option(USE_CPLEX "Build with CPLEX support" ON)
find_package(CPLEX QUIET)

# Main executable
add_executable(test_tsp
    test.cpp
    held_karp.cpp
    branch_and_bound.cpp
    lin_kernighan.cpp
    one_tree.cpp
    brute_force.cpp
    cplex_tsp_solver.cpp
    tsplib_parser.cpp
)

target_include_directories(test_tsp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link CPLEX if available
if(USE_CPLEX AND CPLEX_FOUND)
  add_definitions(-DCPLEX_AVAILABLE)
  target_link_libraries(test_tsp PRIVATE CPLEX::CPLEX)
endif()
