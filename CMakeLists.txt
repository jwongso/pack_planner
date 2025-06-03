cmake_minimum_required(VERSION 3.20)
project(PackPlanner VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    message("Found ccache ${CCACHE_FOUND}")
    message("Using ccache to speed up compilation")
    set(ENV{CCACHE_CPP2} "yes")
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_FOUND})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_FOUND})
endif(CCACHE_FOUND)

# Build types
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    # Detect Apple M1/M2/M3 CPUs
    execute_process(
        COMMAND sysctl -n machdep.cpu.brand_string
        OUTPUT_VARIABLE APPLE_CPU_BRAND
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(STATUS "Detected Apple CPU: ${APPLE_CPU_BRAND}")

    if(APPLE_CPU_BRAND MATCHES "M3")
        set(MARCH_NATIVE "-mcpu=apple-m3")
    elseif(APPLE_CPU_BRAND MATCHES "M2")
        set(MARCH_NATIVE "-mcpu=apple-m2")
    elseif(APPLE_CPU_BRAND MATCHES "M1")
        set(MARCH_NATIVE "-mcpu=apple-m1")
    else()
        message(WARNING "Unknown Apple Silicon CPU, using default flags")
    endif()

elseif(UNIX AND NOT APPLE)
    find_program(LSCPU lscpu)
    if(LSCPU)
        execute_process(
            COMMAND ${LSCPU}
            OUTPUT_VARIABLE LSCPU_OUTPUT
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        string(REGEX MATCH "Model name:[^\n]+" CPU_LINE "${LSCPU_OUTPUT}")
        string(REGEX REPLACE ".*Model name:[ \t]*" "" CPU_MODEL "${CPU_LINE}")

        message(STATUS "Detected CPU model: ${CPU_MODEL}")

        if(CPU_MODEL MATCHES "Zen 2")
            set(MARCH_NATIVE "-march=znver2")
        elseif(CPU_MODEL MATCHES "Zen 3")
            set(MARCH_NATIVE "-march=znver3")
        elseif(CPU_MODEL MATCHES "Zen 4")
            set(MARCH_NATIVE "-march=znver4")
        elseif(CPU_MODEL MATCHES "Alder Lake")
            set(MARCH_NATIVE "-march=alderlake")
        elseif(CPU_MODEL MATCHES "Ryzen AI 9 HX 370")
            set(MARCH_NATIVE "-march=znver5")
        endif()
    endif()
endif()

if(MARCH_NATIVE STREQUAL "")
    set(MARCH_NATIVE "-march=native")
endif()

message(STATUS "Using architecture-specific flags: ${MARCH_NATIVE}")

string(
    APPEND opts
        "$<$<AND:$<CONFIG:RELEASE>,$<COMPILE_LANGUAGE:CXX>>:-Wall;-Wno-deprecated-declarations;-O3;${MARCH_NATIVE};-fPIC;-flto;-fomit-frame-pointer>"
        "$<$<AND:$<CONFIG:DEBUG>,$<COMPILE_LANGUAGE:CXX>>:-Wall;-Wno-deprecated-declarations;-O0;-g3;-D_GLIBCXX_DEBUG;-fPIC>"
        "$<$<AND:$<CONFIG:RELWITHDEBINFO>,$<COMPILE_LANGUAGE:CXX>>:-Wall;-Wno-deprecated-declarations;${MARCH_NATIVE};-O3;-g;-fPIC;-flto;-fomit-frame-pointer>"
        "$<$<AND:$<CONFIG:RELEASE>,$<COMPILE_LANGUAGE:C>>:-Wall;-Wno-deprecated-declarations;${MARCH_NATIVE};-O3;-fPIC;-flto;-fomit-frame-pointer>"
        "$<$<AND:$<CONFIG:DEBUG>,$<COMPILE_LANGUAGE:C>>:-Wall;-Wno-deprecated-declarations;-O0;-g3;-D_GLIBCXX_DEBUG;-fPIC>"
        "$<$<AND:$<CONFIG:RELWITHDEBINFO>,$<COMPILE_LANGUAGE:C>>:-Wall;-Wno-deprecated-declarations;${MARCH_NATIVE};-O3;-g;-fPIC;-flto;-fomit-frame-pointer>"
)

separate_arguments(opts_list UNIX_COMMAND "${opts}")

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
target_compile_options(pack_planner PRIVATE ${opts_list})
set_target_properties(pack_planner PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
)

# Include directories
target_include_directories(pack_planner PRIVATE include)

# Enable threading for benchmarks
find_package(Threads REQUIRED)
target_link_libraries(pack_planner Threads::Threads)

# Enable testing
enable_testing()

# Add tests subdirectory
add_subdirectory(tests)
