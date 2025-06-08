# Pack Planner C#

A high-performance C# pack planning application that efficiently organizes items into packs based on weight and quantity constraints. This is a C# port of the original C++ pack_planner application, maintaining the same functionality while leveraging C#-specific optimizations and modern language features.

## Features

- **Multiple Input Methods**: Read from standard input or file
- **Flexible Sorting**: Support for NATURAL, SHORT_TO_LONG, and LONG_TO_SHORT sorting orders
- **Optimized Algorithms**: O(n) time complexity greedy packing with both blocking and parallel strategies
- **High-Resolution Timing**: Microsecond precision timing utilities using `Stopwatch`
- **Comprehensive Benchmarking**: Built-in performance testing with detailed metrics
- **Item Splitting**: Automatically splits items across packs when needed
- **Thread-Safe Parallel Processing**: Multi-threaded packing strategy for large datasets
- **Input Validation & Safety**: Comprehensive validation of inputs with safety constraints
- **Overflow Protection**: Safe arithmetic operations to prevent integer and floating-point overflow
- **Modern C# Features**: Uses records, pattern matching, nullable reference types, and other C# 9+ features

## Build Requirements

- .NET 9.0 or higher
- C# 12.0 language features

## Building

```bash
cd PackPlannerCSharp/PackPlanner
dotnet build -c Release
```

## Usage

### Basic Usage

```bash
# Read from standard input
dotnet run

# Read from input file
dotnet run -- input.txt

# Run performance benchmark
dotnet run -- --benchmark

# Show help
dotnet run -- --help
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

The C# application achieves excellent performance with optimized greedy packing algorithms:

- **Time Complexity**: O(n) where n is the number of items
- **Space Complexity**: O(n) for storing items and packs
- **Parallel Processing**: Automatic thread scaling based on dataset size
- **Memory Efficiency**: Pre-allocated collections and struct-based items for reduced GC pressure

### C# Specific Optimizations

1. **Struct-based Items**: Using `readonly record struct` for items reduces memory allocations
2. **Span<T> and Memory<T>**: Used where applicable for zero-copy operations
3. **Collection Pre-sizing**: Pre-allocate collections based on empirical ratios
4. **Parallel.ForEach**: Efficient parallel processing for large datasets
5. **ConcurrentBag**: Thread-safe collection for parallel result aggregation
6. **Tiered Compilation**: JIT optimizations for hot paths

## Architecture

### Core Components

- **Item**: Immutable struct representing individual items with ID, length, quantity, and weight
- **Pack**: Container for items with constraint checking and optimization
- **PackPlanner**: Main algorithm implementation with parsing and optimization
- **Timer**: High-resolution timing utility using `Stopwatch`
- **Benchmark**: Performance testing framework
- **Strategy Pattern**: Pluggable packing algorithms (Blocking vs Parallel)

### Packing Strategies

#### Blocking Strategy
- Sequential processing of items
- Single-threaded execution
- Optimal for small datasets (< 5,000 items)
- Lower overhead and memory usage

#### Parallel Strategy
- Multi-threaded processing using Task Parallel Library
- Automatic thread count detection based on processor cores
- Hybrid approach: falls back to sequential for small datasets
- Optimal for large datasets (≥ 5,000 items)

### Algorithm

The packing algorithm uses a greedy approach:

1. **Sorting**: Items are sorted according to the specified order (O(n log n))
2. **Packing**: Items are processed sequentially or in parallel, filling packs optimally (O(n))
3. **Splitting**: Items are automatically split across packs when constraints require it

### Key C# Optimizations

- **Immutable Data Structures**: Using records and readonly structs for thread safety
- **Pattern Matching**: Modern C# switch expressions for cleaner code
- **Nullable Reference Types**: Compile-time null safety
- **Collection Expressions**: Simplified collection initialization
- **Local Functions**: Encapsulated helper methods for better performance
- **Span<T>**: Zero-allocation string parsing where applicable

### Safety Features

- **Input Validation**: All configuration and item inputs are validated for positive values
- **Overflow Protection**: Safe arithmetic operations prevent integer and floating-point overflow
- **Constraint Sanitization**: Configuration values are clamped to safe ranges (e.g., thread count 1-32)
- **Error Handling**: Graceful handling of invalid inputs with appropriate error messages
- **Boundary Checks**: Proper validation of pack constraints and item quantities
- **Zero-Weight Handling**: Special handling for items with zero weight to prevent division by zero

## Project Structure

```
PackPlannerCSharp/
├── PackPlanner/
│   ├── PackPlanner.csproj      # Project configuration with optimizations
│   ├── Program.cs              # Main entry point and input parsing
│   ├── Item.cs                 # Immutable item struct
│   ├── Pack.cs                 # Pack container with constraint checking
│   ├── PackPlanner.cs          # Main planning algorithm and configuration
│   ├── SortOrder.cs            # Sorting enumeration and extensions
│   ├── Timer.cs                # High-resolution timing utility
│   ├── IPackStrategy.cs        # Strategy pattern interface and factory
│   ├── BlockingPackStrategy.cs # Sequential packing implementation
│   ├── ParallelPackStrategy.cs # Parallel packing implementation
│   ├── Benchmark.cs            # Performance testing framework
README.md                       # This file
```

## Differences from C++ Version

### Improvements
1. **Memory Safety**: No manual memory management, automatic garbage collection
2. **Type Safety**: Stronger type system with nullable reference types
3. **Modern Language Features**: Records, pattern matching, LINQ, async/await ready
4. **Better Error Handling**: Exception-based error handling with try-catch blocks
5. **Immutable Data**: Thread-safe immutable structs and records
6. **Automatic Resource Management**: Using statements for automatic disposal

### Performance Considerations
1. **Garbage Collection**: Managed memory with potential GC pauses
2. **JIT Compilation**: Initial warmup time, but optimized hot paths
3. **Boxing/Unboxing**: Minimized through generic collections and structs
4. **Virtual Calls**: Interface calls have slight overhead vs C++ templates

### Architectural Differences
1. **Namespace Organization**: Proper C# namespace structure
2. **Property-based APIs**: C# properties instead of getter/setter methods
3. **LINQ Integration**: Functional programming capabilities
4. **Exception Handling**: Structured exception handling vs error codes
5. **Async-Ready**: Designed for potential async/await integration

## Performance Comparison

To compare performance between C++ and C# versions:

```bash
# Build and run C++ version
cd /path/to/cpp/pack_planner
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
./pack_planner --benchmark

# Build and run C# version
cd /path/to/PackPlannerCSharp/PackPlanner
dotnet build -c Release
dotnet run -c Release -- --benchmark
```

## License

This project is provided as-is for educational and evaluation purposes.

## Development Notes

### C# Specific Design Decisions

1. **Record Types**: Used for configuration and result types for immutability and value semantics
2. **Struct vs Class**: Items are structs for performance, Packs are classes for reference semantics
3. **Interface Segregation**: Clean separation between strategy interface and implementations
4. **Extension Methods**: Used for enum operations to keep code organized
5. **Nullable Reference Types**: Enabled for compile-time null safety
6. **Collection Initialization**: Modern C# collection initialization patterns

### Performance Tuning

1. **Avoid Allocations**: Pre-size collections, use structs for small data
2. **Minimize Boxing**: Use generic collections and avoid object parameters
3. **Efficient Loops**: Use for loops over foreach where performance critical
4. **Parallel Processing**: Automatic scaling based on data size and processor count
5. **Memory Pooling**: Consider ArrayPool<T> for future optimizations

### Future Enhancements

1. **Async/Await**: Potential for async file I/O and parallel processing
2. **Memory Pooling**: ArrayPool<T> for reduced allocations
3. **SIMD Operations**: Vector<T> for mathematical operations
4. **Native AOT**: Ahead-of-time compilation for faster startup
5. **Benchmarking**: Integration with BenchmarkDotNet for detailed performance analysis
