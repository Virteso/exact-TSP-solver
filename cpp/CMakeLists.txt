cmake_minimum_required(VERSION 3.10)
project(TSP_Solver)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")
endif()

# Add cmake directory to module path
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Build LKH-3 as a static library
set(LKH3_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../LKH-3)
set(LKH3_IDIR ${LKH3_DIR}/INCLUDE)

file(GLOB LKH3_SOURCES
    "${LKH3_DIR}/*.c"
)
# Exclude LKHmain.c (has its own main())
list(FILTER LKH3_SOURCES EXCLUDE REGEX ".*LKHmain\\.c$")

add_library(lkh3 STATIC ${LKH3_SOURCES})
target_include_directories(lkh3 PRIVATE ${LKH3_IDIR})
target_compile_options(lkh3 PRIVATE -DTWO_LEVEL_TREE -fcommon -w -g -O0)
target_link_libraries(lkh3 PRIVATE m)

# Check for CPLEX
option(USE_CPLEX "Build with CPLEX support" ON)
find_package(CPLEX QUIET)

# Main executable
add_executable(test_tsp
    test.cpp
    tsplib_parser.cpp
    brute_force.cpp
    held_karp.cpp
    branch_and_bound.cpp
    one_tree.cpp
    cplex_tsp_solver.cpp
    lkh_solver.cpp
)

target_include_directories(test_tsp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${LKH3_IDIR})
target_link_libraries(test_tsp PRIVATE lkh3)

# Link CPLEX if available
if(USE_CPLEX AND CPLEX_FOUND)
  add_definitions(-DCPLEX_AVAILABLE)
  target_link_libraries(test_tsp PRIVATE CPLEX::CPLEX)
endif()
