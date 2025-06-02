# Pack Planner

A high-performance C++20 pack planning application that efficiently organizes items into packs based on weight and quantity constraints.

## Features

- **Multiple Input Methods**: Read from standard input or file
- **Flexible Sorting**: Support for NATURAL, SHORT_TO_LONG, and LONG_TO_SHORT sorting orders
- **Optimized Algorithm**: O(n) time complexity for optimal performance
- **High-Resolution Timing**: Microsecond precision timing utilities
- **Comprehensive Benchmarking**: Built-in performance testing with detailed metrics
- **Item Splitting**: Automatically splits items across packs when needed

## Build Requirements

- C++20 compatible compiler (GCC 10+, Clang 10+)
- CMake 3.20 or higher
- Threading support

## Building

```bash
mkdir build
cd build
cmake ..
make
```

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
Size    Sorting(ms) Packing(ms) Total(ms)   Items/sec   Packs   Util%
----------------------------------------------------------------------
10000   NAT         1.728       1.729       292342394   13878   76.2%
100000  NAT         14.486      14.487      348656450   138905  76.5%
500000  NAT         56.977      56.977      443384067   694672  76.4%
1000000 NAT         112.698     112.699     448072387   1388287 76.4%
2000000 NAT         160.616     160.617     628945155   2777661 76.4%
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
