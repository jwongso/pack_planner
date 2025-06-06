# Pack Planner

A high-performance C++20 pack planning application that efficiently organizes items into packs based on weight and quantity constraints.

## Features

- **Multiple Input Methods**: Read from standard input or file
- **Flexible Sorting**: Support for NATURAL, SHORT_TO_LONG, and LONG_TO_SHORT sorting orders
- **Optimized Algorithm**: O(n) time complexity for optimal performance
- **High-Resolution Timing**: Microsecond precision timing utilities
- **Comprehensive Benchmarking**: Built-in performance testing with detailed metrics
- **Item Splitting**: Automatically splits items across packs when needed
- **Emscripten or WebAssembly**: Support web client with efficient on-client computation via WebAssembly

## Safety Constraints

To prevent numerical overflows or memory exhaustion, the following practical limits are used internally:

```cpp
constexpr int SAFE_MAX_LENGTH   = 1'000'000;
constexpr int SAFE_MAX_QUANTITY = 10'000;
constexpr double SAFE_MAX_WEIGHT = 1e6;
```

Items exceeding these limits are ignored during the packing process to maintain stability and ensure safe memory usage.

## Build Requirements

- C++20 compatible compiler (GCC 10+, Clang 10+)
- CMake 3.20 or higher
- Threading support

## Build Requirements (WebAssembly)

- Emscripten SDK (https://emscripten.org/docs/getting_started/downloads.html)
- Python 3.x

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Building Wasm (WebAssembly)

```bash
emcmake cmake -B build-wasm -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm
cd build-wasm
cp ../index.html ../server.py .
python server.py # To run the simple web server
```

Open http://localhost:8000 on your local web browser application.

## Usage

### Basic Usage

```bash
# Read from standard input
./pack_planner

# Read from input file
./pack_planner input.txt

# Run performance benchmark
./pack_planner --benchmark

# Show help
./pack_planner --help
```

### Input Format

```
[Sort order],[max pieces per pack],[max weight per pack]
[item id],[item length],[item quantity],[piece weight]
[item id],[item length],[item quantity],[piece weight]
...
```

**Sort Orders:**
- `NATURAL`: Keep original order
- `SHORT_TO_LONG`: Sort by length ascending
- `LONG_TO_SHORT`: Sort by length descending

### Example Input

```
NATURAL,40,500.0
1001,6200,30,9.653
2001,7200,50,11.21
```

### Example Output

```
Pack Number: 1
1001,6200,30,9.653
2001,7200,10,11.210
Pack Length: 7200, Pack Weight: 401.69
Pack Number: 2
2001,7200,40,11.210
Pack Length: 7200, Pack Weight: 448.40
```

## Performance

The application achieves excellent performance with the optimized greedy packing algorithm:

- **Time Complexity**: O(n) where n is the number of items
- **Space Complexity**: O(n) for storing items and packs
- **Throughput**: Up to 600+ million items/second on modern hardware
- **Utilization**: Typically achieves 76%+ pack utilization

### Benchmark Results

```
=== PERFORMANCE BENCHMARK ===
Running C++ Performance Benchmarks...
Size    Sorting(ms) Packing(ms) Total(ms)   Items/sec   Packs   Util%
----------------------------------------------------------------------
100000  NAT         23.765      23.765      212538859   138905  76.5%
1000000 NAT         155.143     155.144     325486709   1388287 76.4%
5000000 NAT         623.928     623.929     404753156   6943771 76.4%
10000000NAT         1110.229    1110.230    454928201   1388731376.4%
20000000NAT         2238.344    2238.345    451237911   2777055376.4%

Size    Sorting(ms) Packing(ms) Total(ms)   Items/sec   Packs   Util%
----------------------------------------------------------------------
100000  LTS         3.530       7.458       677257441   138930  76.4%
1000000 LTS         35.605      73.542      686645862   1388260 76.4%
5000000 LTS         167.821     364.307     693198955   6943585 76.4%
10000000LTS         331.810     722.653     698917650   1388692076.4%
20000000LTS         1331.847    2125.375    475222547   2777047676.4%

Size    Sorting(ms) Packing(ms) Total(ms)   Items/sec   Packs   Util%
----------------------------------------------------------------------
100000  STL         3.820       7.689       656910651   138895  76.5%
1000000 STL         37.685      75.821      666006911   1388268 76.4%
5000000 STL         167.354     351.287     718891481   6943636 76.4%
10000000STL         334.184     711.041     710331664   1388718876.4%
20000000STL         1327.421    2098.114    481397160   2777054876.4%

Total benchmark execution: 14283.868 ms (14283868 μs)
```

## Architecture

### Core Components

- **Item**: Represents individual items with ID, length, quantity, and weight
- **Pack**: Container for items with constraint checking
- **PackPlanner**: Main algorithm implementation with parsing and optimization
- **Timer**: High-resolution timing utility using `std::chrono`
- **Benchmark**: Performance testing framework

### Algorithm

The packing algorithm uses a greedy approach:

1. **Sorting**: Items are sorted according to the specified order (O(n log n))
2. **Packing**: Items are processed sequentially, filling packs optimally (O(n))
3. **Splitting**: Items are automatically split across packs when constraints require it

### Key Optimizations

- **Single-pass packing**: Each item is processed exactly once
- **Efficient constraint checking**: Minimal computational overhead
- **Memory optimization**: Minimal memory allocations during packing
- **Cache-friendly**: Sequential access patterns for better performance

## Project Structure

```
pack_planner/
├── CMakeLists.txt
├── README.md
├── input.txt                 # Sample input file
├── include/
│   ├── Benchmark.h
│   ├── Item.h
│   ├── Pack.h
│   ├── PackPlanner.h
│   ├── SortOrder.h
│   └── Timer.h
└── src/
    ├── Benchmark.cpp
    ├── Item.cpp
    ├── main.cpp
    ├── Pack.cpp
    ├── PackPlanner.cpp
    └── Timer.cpp
```

## License

This project is provided as-is for educational and evaluation purposes.
