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

## Testing
### Tricky Test Cases for Pack Planner

#### 1. Weight-Constrained Infinite Loop (Your Original Issue)
```
NATURAL,20,300.0
1001,6200,50,9.653
2001,7200,90,11.21
3001,8200,50,19.653
4001,9200,90,21.21
5001,16200,50,29.653
6001,17200,90,31.21
```
**Problem**: Items are heavy relative to pack weight capacity. Packs fill up by weight before reaching item count limit.

#### 2. Item-Count-Constrained Case
```
NATURAL,5,1000.0
1001,100,20,1.0
2001,200,30,2.0
3001,300,25,1.5
```
**Problem**: Packs fill up by item count before reaching weight limit. Tests the item count constraint.

#### 3. Single Heavy Item That Can't Fit
```
NATURAL,10,100.0
1001,5000,1,150.0
2001,1000,5,10.0
```
**Problem**: First item weighs 150.0 but max pack weight is 100.0. Should cause infinite loop without proper handling.

#### 4. Exact Boundary Conditions
```
NATURAL,10,100.0
1001,1000,10,10.0
2001,2000,5,20.0
```
**Problem**: First item exactly fills one pack (10 items × 10.0 weight = 100.0). Second item exactly fills half a pack.

#### 5. Fractional Weight Precision Issues
```
NATURAL,3,10.0
1001,100,10,3.333333
2001,200,5,1.666667
```
**Problem**: 3 × 3.333333 = 9.999999, which might be less than 10.0 due to floating-point precision, but 4 items would exceed the limit.

#### 6. Zero/Negative Values (Should Throw Exceptions)
```
NATURAL,10,100.0
1001,1000,0,10.0
2001,2000,-5,20.0
3001,3000,5,-10.0
```
**Problem**: Invalid item parameters should be caught by validation.

#### 7. Large Quantity with Tiny Weight
```
NATURAL,1000,50.0
1001,100,10000,0.001
```
**Problem**: 10,000 items of 0.001 weight each = 10.0 total weight, but exceeds item count limit of 1000.

#### 8. Sorting Order Impact
```
LONG_TO_SHORT,5,100.0
1001,1000,3,30.0
2001,5000,2,35.0
3001,2000,4,25.0
```
**Problem**: Different sorting orders should produce different packing results. Tests if sorting actually affects the algorithm.

#### 9. Many Small Items + One Large Item
```
NATURAL,20,100.0
1001,100,50,1.0
2001,10000,1,90.0
```
**Problem**: Large item dominates pack weight, leaving little room for small items in the same pack.

#### 10. Edge Case: Empty Input After Header
```
NATURAL,10,100.0
```
**Problem**: No items to pack. Should handle gracefully without crashing.

#### 11. Stress Test: Many Items, Tight Constraints
```
SHORT_TO_LONG,3,15.0
1001,100,10,2.0
1002,200,8,3.0
1003,300,6,4.0
1004,400,4,5.0
1005,500,2,6.0
1006,600,1,7.0
```
**Problem**: Each item type has different weight, quantity constraints are very tight.

#### 12. All Items Same Weight/Length (Degenerate Case)
```
NATURAL,5,25.0
1001,1000,20,5.0
2001,1000,15,5.0
3001,1000,10,5.0
```
**Problem**: All items identical except ID and quantity. Tests basic packing logic without sorting complexity.

#### 13. Rounding Issues with Weight Calculations
```
NATURAL,10,10.0
1001,100,7,1.428571428571
2001,200,3,3.333333333333
```
**Problem**: 7 × 1.428571428571 ≈ 10.0, but floating-point arithmetic might cause issues.

### Expected Behaviors:
- **Cases 1, 3**: Should cause infinite loops without proper fixes
- **Case 6**: Should throw exceptions due to invalid parameters
- **Case 10**: Should handle empty input gracefully
- **Cases 2, 4, 7, 8, 11**: Should test different constraint boundaries
- **Cases 5, 13**: Should reveal floating-point precision issues
- **Cases 9, 12**: Should test algorithm efficiency and correctness

### Testing Strategy:
1. Run each test case and verify no infinite loops
2. Check that all items are properly distributed
3. Verify pack constraints are respected
4. Compare results between different sorting orders
5. Ensure error handling works for invalid inputs

## License

This project is provided as-is for educational and commercial use.
