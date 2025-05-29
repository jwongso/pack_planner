cmake_minimum_required(VERSION 3.10)
project(PackPlanner VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler-specific options
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
endif()

# Build types
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# Header files
set(HEADERS
    src/item.h
    src/pack.h
    src/timer.h
    src/sorter.h
    src/input_parser.h
    src/pack_planner.h
    src/pack_planner_app.h
    src/benchmark.h
)

# Source files
set(SOURCES
    main.cpp
)

# Create the executable
add_executable(pack_planner ${SOURCES} ${HEADERS})

# Set target properties
set_target_properties(pack_planner PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Include current directory for headers
target_include_directories(pack_planner PRIVATE
                            ${CMAKE_CURRENT_SOURCE_DIR}
                            ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Custom targets for testing and benchmarking
add_custom_target(test_run
    COMMAND ${CMAKE_COMMAND} -E echo "Running basic functionality test..."
    COMMAND ${CMAKE_COMMAND} -E echo "NATURAL,50,500.0" > test_input.txt
    COMMAND ${CMAKE_COMMAND} -E echo "1001,1000,10,5.0" >> test_input.txt
    COMMAND ${CMAKE_COMMAND} -E echo "1002,2000,5,3.0" >> test_input.txt
    COMMAND ${CMAKE_COMMAND} -E echo "" >> test_input.txt
    COMMAND pack_planner test_input.txt
    COMMAND ${CMAKE_COMMAND} -E remove test_input.txt
    DEPENDS pack_planner
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running pack planner test"
)

add_custom_target(benchmark_run
    COMMAND pack_planner --benchmark
    DEPENDS pack_planner
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running performance benchmark"
)

# Installation
install(TARGETS pack_planner
    RUNTIME DESTINATION bin
)

# Optional: Package configuration
set(CPACK_PACKAGE_NAME "PackPlanner")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High-performance pack planning application")
set(CPACK_PACKAGE_VENDOR "PackPlanner Team")

include(CPack)

# Print build information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug flags: ${CMAKE_CXX_FLAGS_DEBUG}")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Release flags: ${CMAKE_CXX_FLAGS_RELEASE}")
endif()
