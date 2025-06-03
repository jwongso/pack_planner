cmake_minimum_required(VERSION 3.20)

# Find GTest package
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Create test executable
add_executable(pack_planner_tests
    pack_planner_tests.cpp
    item_test.cpp
    pack_test.cpp
)

# Link against GTest and the main project
target_link_libraries(pack_planner_tests
    ${GTEST_LIBRARIES}
    ${GTEST_MAIN_LIBRARIES}
    Threads::Threads
)

# Include directories
target_include_directories(pack_planner_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Add test to CTest
add_test(NAME PackPlannerTests COMMAND pack_planner_tests)
