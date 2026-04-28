# FindCPLEX.cmake - Find CPLEX libraries and headers

# Try to find CPLEX_ROOT_DIR
if(NOT DEFINED CPLEX_ROOT_DIR)
  set(CPLEX_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../cplex" CACHE PATH "CPLEX root directory")
endif()

find_path(CPLEX_INCLUDE_DIRS
    NAMES ilcplex/ilocplex.h
    PATHS ${CPLEX_ROOT_DIR}/cplex/include
)

find_library(ILOCPLEX_LIBRARY
    NAMES ilocplex
    PATHS ${CPLEX_ROOT_DIR}/cplex/lib/*/static_pic
)

find_library(CPLEX_LIBRARY
    NAMES cplex
    PATHS ${CPLEX_ROOT_DIR}/cplex/lib/*/static_pic
)

find_library(CONCERT_LIBRARY
    NAMES concert
    PATHS ${CPLEX_ROOT_DIR}/concert/lib/*/static_pic
)

if(CPLEX_INCLUDE_DIRS AND ILOCPLEX_LIBRARY AND CPLEX_LIBRARY AND CONCERT_LIBRARY)
    set(CPLEX_FOUND TRUE)
    add_library(CPLEX::CPLEX INTERFACE IMPORTED)
    set_target_properties(CPLEX::CPLEX PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CPLEX_INCLUDE_DIRS};${CPLEX_ROOT_DIR}/concert/include"
        INTERFACE_LINK_LIBRARIES "${ILOCPLEX_LIBRARY};${CPLEX_LIBRARY};${CONCERT_LIBRARY}"
        INTERFACE_COMPILE_OPTIONS "-fPIC"
    )
else()
    set(CPLEX_FOUND FALSE)
    message(STATUS "CPLEX not found. To enable CPLEX support:")
    message(STATUS "  1. Set CPLEX_ROOT_DIR to your CPLEX installation root")
    message(STATUS "  2. Build with: cmake -DUSE_CPLEX=ON -DCPLEX_ROOT_DIR=/path/to/cplex ..")
endif()
