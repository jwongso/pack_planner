cmake_minimum_required(VERSION 3.20)
project(PackPlanner VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ... existing ccache and build type configuration ...

# ... existing architecture detection and compiler flags ...

# Source files
set(SOURCES
    src/main.cpp
    src/benchmark.cpp
)

# Header files
set(HEADERS
    include/item.h
    include/pack.h
    include/pack_planner.h
    include/timer.h
    include/benchmark.h
    include/sort_order.h
)

# Create executable
add_executable(pack_planner ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(pack_planner PRIVATE include)

# Enable threading for benchmarks
find_package(Threads REQUIRED)
target_link_libraries(pack_planner Threads::Threads)

# Enable testing
enable_testing()

# Add tests subdirectory
add_subdirectory(tests)
