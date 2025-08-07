cmake_minimum_required(VERSION 3.20)
project(pack_planner VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_path(CONCURRENTQUEUE_INCLUDE_DIR
    NAMES moodycamel/concurrentqueue.h
    PATHS /usr/include/concurrentqueue
          /usr/local/include/concurrentqueue
          /usr/include
          /usr/local/include
)

# Check if we're building with Emscripten
if(EMSCRIPTEN)
    message(STATUS "Building with Emscripten for WebAssembly")
    set(WASM_BUILD TRUE)
    set(CMAKE_EXECUTABLE_SUFFIX ".js")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -pthread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

    # Emscripten-specific compiler flags
    set(EMSCRIPTEN_FLAGS
        "-s WASM=1"
        "-s ALLOW_MEMORY_GROWTH=1"
        "-s EXPORTED_RUNTIME_METHODS=[\"ccall\",\"cwrap\"]"
        "-s EXPORT_ES6=1"
        "-s MODULARIZE=1"
        "-s ASSERTIONS"
        "-s PTHREAD_POOL_SIZE=8"
        "-s INITIAL_MEMORY=128MB"
        "--bind"
        "-O3"
    )
    string(REPLACE ";" " " EMSCRIPTEN_FLAGS_STR "${EMSCRIPTEN_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EMSCRIPTEN_FLAGS_STR}")
else()
    set(WASM_BUILD FALSE)

    include(FetchContent)
    FetchContent_Declare(
        cli11
        GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
        GIT_TAG v2.3.2
    )
    FetchContent_MakeAvailable(cli11)

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
                # Zen 5 specific optimizations
                set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mprefer-vector-width=512")
                # Enable AVX-512 if supported
                set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mavx512f -mavx512vl")
            endif()
        endif()
    endif()

    if(MARCH_NATIVE STREQUAL "")
        set(MARCH_NATIVE "-march=native")
    endif()

    message(STATUS "Using architecture-specific flags: ${MARCH_NATIVE}")

    string(
        APPEND opts
            "$<$<AND:$<CONFIG:RELEASE>,$<COMPILE_LANGUAGE:CXX>>:-Wall;-Wno-deprecated-declarations;-O3;${MARCH_NATIVE};-fPIC;-flto;-fomit-frame-pointer;-funroll-loops;-fprefetch-loop-arrays;-mprefer-vector-width=256;-falign-functions=32;-falign-loops=32>"
            "$<$<AND:$<CONFIG:DEBUG>,$<COMPILE_LANGUAGE:CXX>>:-Wall;-Wno-deprecated-declarations;-O0;-g3;-fPIC>"
            "$<$<AND:$<CONFIG:RELWITHDEBINFO>,$<COMPILE_LANGUAGE:CXX>>:-Wall;-Wno-deprecated-declarations;${MARCH_NATIVE};-O3;-g;-fPIC;-flto;-fomit-frame-pointer>"
            "$<$<AND:$<CONFIG:RELEASE>,$<COMPILE_LANGUAGE:C>>:-Wall;-Wno-deprecated-declarations;${MARCH_NATIVE};-O3;-fPIC;-flto;-fomit-frame-pointer>"
            "$<$<AND:$<CONFIG:DEBUG>,$<COMPILE_LANGUAGE:C>>:-Wall;-Wno-deprecated-declarations;-O0;-g3;-fPIC>"
            "$<$<AND:$<CONFIG:RELWITHDEBINFO>,$<COMPILE_LANGUAGE:C>>:-Wall;-Wno-deprecated-declarations;${MARCH_NATIVE};-O3;-g;-fPIC;-flto;-fomit-frame-pointer>"
    )

    separate_arguments(opts_list UNIX_COMMAND "${opts}")
endif()

# Source files
set(SOURCES
    src/pack_strategy_factory.cpp
)

# Header files
set(HEADERS
    include/item.h
    include/pack.h
    include/pack_planner.h
    include/timer.h
    include/sort_order.h
    include/pack_strategy.h
    include/blocking_pack_strategy.h
    include/parallel_pack_strategy.h
    include/lockfree_pack_strategy.h
    include/optimized_sort.h
    include/blocking_next_fit_strategy.h
)

# WebAssembly specific files
if(WASM_BUILD)
    list(APPEND SOURCES
        src/wasm_bindings.cpp
        src/benchmark.cpp
    )
    list(APPEND HEADERS
        include/wasm_bindings.h
        include/benchmark.h
    )
endif()

# Create library
add_library(${PROJECT_NAME}_LIB ${SOURCES} ${HEADERS})

if(WASM_BUILD)
    # Create WebAssembly module
    add_executable(${PROJECT_NAME}_wasm src/wasm_bindings.cpp)
    target_include_directories(${PROJECT_NAME}_wasm PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${CONCURRENTQUEUE_INCLUDE_DIR}
    )
    target_link_libraries(${PROJECT_NAME}_wasm ${PROJECT_NAME}_LIB)

    set(FILES_TO_COPY server.py index.html dashboard.html wasm_profiler.html styles.css app.js)
    foreach(f ${FILES_TO_COPY})
        add_custom_command(
            TARGET ${PROJECT_NAME}_wasm POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_SOURCE_DIR}/${f}
                    $<TARGET_FILE_DIR:${PROJECT_NAME}_wasm>/${f}
        )
    endforeach()
else()
    # Regular executable for native builds
    set(MAIN_SRC
        src/main.cpp
        src/benchmark.cpp
    )

    set(MAIN_INCLUDE
        include/benchmark.h
    )

    add_executable(${PROJECT_NAME} ${MAIN_SRC} ${MAIN_INCLUDE})
    target_compile_options(${PROJECT_NAME} PRIVATE ${opts_list})
    target_compile_options(${PROJECT_NAME}_LIB PRIVATE ${opts_list})

    set_target_properties(${PROJECT_NAME} PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(pack_planner PRIVATE -mavx2 -mfma)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        target_compile_options(pack_planner PRIVATE /arch:AVX2)
    endif()

    # Include directories
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${CONCURRENTQUEUE_INCLUDE_DIR}
    )

    # Enable threading for benchmarks
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_LIB Threads::Threads)

    # Enable testing
    enable_testing()

    # Add tests subdirectory
    add_subdirectory(tests)
endif()

target_include_directories(${PROJECT_NAME}_LIB PRIVATE ${PROJECT_SOURCE_DIR}/include)
