# Pack Planner Application

A high-performance C++ application for optimizing item packing with integrated benchmarking and timing capabilities.

## Project Structure

```
├── CMakeLists.txt          # CMake build configuration
├── build.sh               # Build script
├── main.cpp               # Main application entry point
src
├── item.h                 # Item class definition
├── pack.h                 # Pack class definition
├── timer.h                # High-resolution timing utilities
├── sorter.h               # Sorting strategies and factory
├── input_parser.h         # Input parsing utilities
├── pack_planner.h         # Core pack planning algorithm
├── pack_planner_app.h     # Application framework
└── benchmark.h            # Benchmarking and performance testing
test
```

## Features

- **Modular Design**: Clean separation of concerns with header-only components
- **Integrated Timing**: Automatic timing of all operations (parsing, sorting, packing, output)
- **Multiple Sort Orders**: Natural, short-to-long, long-to-short sorting strategies
- **Performance Benchmarking**: Built-in benchmark suite for performance analysis
- **Flexible Input**: Console input or file-based input
- **Cross-Platform**: CMake-based build system for Windows, Linux, and macOS

## Building

### Prerequisites
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10 or later

### Quick Build (Linux/macOS)
```bash
chmod +x build.sh
./build.sh                 # Release build
./build.sh Debug           # Debug build
./build.sh Release test    # Build and run test
./build.sh Release benchmark # Build and run benchmark
```

### Manual CMake Build
```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

### Windows (Visual Studio)
```cmd
mkdir build
cd build
cmake -G "Visual Studio 16 2019" ..
cmake --build . --config Release
```

## Usage

### Basic Usage
```bash
# Console input (end with empty line)
./pack_planner

# File input
./pack_planner input.txt

# Quiet mode (no timing output)
./pack_planner --quiet input.txt

# Performance benchmark
./pack_planner --benchmark
```

### Input Format
```
SORT_ORDER,MAX_ITEMS,MAX_WEIGHT
ITEM_ID,LENGTH,QUANTITY,WEIGHT
ITEM_ID,LENGTH,QUANTITY,WEIGHT
...
```

Example:
```
SHORT_TO_LONG,50,500.0
1001,1000,10,5.0
1002,2000,5,3.0
1003,1500,8,2.5
```

### Sort Orders
- `NATURAL`: Keep original input order
- `SHORT_TO_LONG`: Sort by length ascending
- `LONG_TO_SHORT`: Sort by length descending

## Timing Output

The application provides detailed timing information:

```
=== TIMING INFORMATION ===
Input parsing: 0.125 ms (125 μs)
Sorting: 0.043 ms (43 μs)
Packing: 1.234 ms (1234 μs)
Total algorithm time: 1.277 ms (1277 μs)
Items processed: 1000
Packs created: 23
Performance: 783123 items/second
=========================

Output generation: 2.456 ms (2456 μs)

Total execution: 3.858 ms (3858 μs)
```

## Benchmarking

Run comprehensive performance tests:
```bash
./pack_planner --benchmark
```

Sample benchmark output:
```
=== PERFORMANCE BENCHMARK ===
Running C++ Performance Benchmarks...
Size    Sorting(ms)    Packing(ms)    Total(ms)    Items/sec
----    -----------    -----------    ---------    ---------
100     0.012          0.089          0.145        689655
1000    0.098          0.756          0.932        1072961
5000    0.445          3.234          4.012        1246266
10000   0.912          6.789          8.123        1231066
50000   4.567          34.123         41.234       1213045
=============================
```

## CMake Targets

- `pack_planner`: Main executable
- `test_run`: Run basic functionality test
- `benchmark_run`: Run performance benchmark
- `install`: Install to system

```bash
# Run specific targets
cmake --build . --target test_run
cmake --build . --target benchmark_run
```

## Performance Characteristics

- **High Throughput**: Processes 1M+ items/second on modern hardware
- **Memory Efficient**: Minimal memory allocation during packing
- **Scalable**: Linear time complexity for most operations
- **Optimized**: Compiler optimizations enabled in Release mode

## Development

### Debug Build
```bash
./build.sh Debug
```

### Adding New Features
The modular design makes it easy to extend:
- Add new sorting strategies in `sorter.h`
- Extend item properties in `item.h`
- Add new packing algorithms by inheriting from `pack_planner`
- Extend benchmarking in `benchmark.h`

## License

This project is provided as-is for educational and commercial use.
