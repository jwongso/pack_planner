# Pack Planner C#

A high-performance C# pack planning solution that efficiently organizes items into packs based on weight and quantity constraints. This solution includes both a console application and a modern Web API, providing flexible deployment options for different use cases. This is a C# port of the original C++ pack_planner application, maintaining the same functionality while leveraging C#-specific optimizations and modern language features.

## Features

### Core Pack Planning Features
- **Multiple Input Methods**: Read from standard input or file (console app)
- **Flexible Sorting**: Support for NATURAL, SHORT_TO_LONG, and LONG_TO_SHORT sorting orders
- **Optimized Algorithms**: O(n) time complexity greedy packing with both blocking and parallel strategies
- **High-Resolution Timing**: Microsecond precision timing utilities using `Stopwatch`
- **Comprehensive Benchmarking**: Built-in performance testing with detailed metrics
- **Item Splitting**: Automatically splits items across packs when needed
- **Thread-Safe Parallel Processing**: Multi-threaded packing strategy for large datasets
- **Input Validation & Safety**: Comprehensive validation of inputs with safety constraints
- **Overflow Protection**: Safe arithmetic operations to prevent integer and floating-point overflow
- **Modern C# Features**: Uses records, pattern matching, nullable reference types, and other C# 9+ features

### Web API Features
- **RESTful API**: Modern HTTP API with OpenAPI/Swagger documentation
- **Async Processing**: Support for both synchronous and asynchronous pack planning
- **Rate Limiting**: Built-in rate limiting to prevent API abuse
- **Health Checks**: Comprehensive health monitoring endpoints
- **CORS Support**: Cross-origin resource sharing for web applications
- **Input Validation**: Comprehensive request validation with detailed error messages
- **Performance Metrics**: Detailed timing and utilization metrics in responses
- **Auto Thread Detection**: Automatic optimal thread count detection based on system capabilities

## Build Requirements

- .NET 9.0 or higher
- C# 12.0 language features

## Building

### Console Application
```bash
cd PackPlannerCSharp/PackPlanner
dotnet build -c Release
```

### Web API
```bash
cd PackPlannerCSharp/PackPlanner.Api
dotnet build -c Release
```

### Build All Projects
```bash
cd PackPlannerCSharp
dotnet build -c Release
```

## Usage

### Console Application

```bash
cd PackPlannerCSharp/PackPlanner

# Read from standard input
dotnet run

# Read from input file
dotnet run -- input.txt

# Run performance benchmark
dotnet run -- --benchmark

# Show help
dotnet run -- --help
```

### Web API

```bash
cd PackPlannerCSharp/PackPlanner.Api

# Run the API server
dotnet run

# Run in production mode
dotnet run --environment Production

# The API will be available at:
# - HTTP: http://localhost:5000
# - HTTPS: https://localhost:5001
# - Swagger UI: https://localhost:5001/swagger
```

#### API Endpoints

**Core Pack Planning:**
- `POST /api/packplanner/plan` - Synchronous pack planning
- `POST /api/packplanner/plan-async` - Asynchronous pack planning
- `GET /api/packplanner/jobs/{jobId}` - Get async job status
- `GET /api/packplanner/jobs/{jobId}/result` - Get async job result
- `DELETE /api/packplanner/jobs/{jobId}` - Cancel/delete async job

**Information Endpoints:**
- `GET /api/packplanner/strategies` - Get available packing strategies
- `GET /api/packplanner/sort-orders` - Get available sort orders
- `GET /api/packplanner/rate-limit-info` - Get rate limiting information

**System Endpoints:**
- `GET /health` - Health check
- `GET /api/packplanner/version` - API version information

#### Example API Request

```json
POST /api/packplanner/plan
Content-Type: application/json

{
  "items": [
    {
      "id": 1001,
      "length": 6200,
      "quantity": 30,
      "weight": 9.653
    },
    {
      "id": 2001,
      "length": 7200,
      "quantity": 50,
      "weight": 11.21
    }
  ],
  "configuration": {
    "sortOrder": "NATURAL",
    "maxItemsPerPack": 40,
    "maxWeightPerPack": 500.0,
    "strategyType": "BLOCKING",
    "threadCount": 0
  }
}
```

#### API Validation Limits

The API enforces the following validation constraints for safety and performance:

**Item Constraints:**
- `id`: Any integer (including 0)
- `length`: 1 to 1,000,000
- `quantity`: 1 to 100,000
- `weight`: 0.001 to 10,000

**Configuration Constraints:**
- `maxItemsPerPack`: 1 to 10,000
- `maxWeightPerPack`: 0.1 to 100,000
- `threadCount`: 0 to 32 (0 = auto-detect processor count)
- `sortOrder`: "NATURAL", "SHORT_TO_LONG", "LONG_TO_SHORT"
- `strategyType`: "BLOCKING", "PARALLEL"

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
├── PackPlanner/                    # Console Application
│   ├── PackPlanner.csproj          # Project configuration with optimizations
│   ├── Program.cs                  # Main entry point and input parsing
│   ├── Item.cs                     # Immutable item struct
│   ├── Pack.cs                     # Pack container with constraint checking
│   ├── PackPlanner.cs              # Main planning algorithm and configuration
│   ├── SortOrder.cs                # Sorting enumeration and extensions
│   ├── Timer.cs                    # High-resolution timing utility
│   ├── IPackStrategy.cs            # Strategy pattern interface and factory
│   ├── BlockingPackStrategy.cs     # Sequential packing implementation
│   ├── ParallelPackStrategy.cs     # Parallel packing implementation
│   └── Benchmark.cs                # Performance testing framework
├── PackPlanner.Api/                # Web API Application
│   ├── PackPlanner.Api.csproj      # API project configuration
│   ├── Program.cs                  # API startup and configuration
│   ├── Controllers/
│   │   └── PackPlannerController.cs # Main API controller
│   ├── Models/
│   │   └── ApiModels.cs            # Request/response models with validation
│   ├── Middleware/
│   │   └── RateLimitingMiddleware.cs # Rate limiting implementation
│   ├── Configuration/
│   │   └── RateLimitingOptions.cs  # Rate limiting configuration
│   ├── Properties/
│   │   └── launchSettings.json     # Development server settings
│   ├── appsettings.json            # Application configuration
│   └── appsettings.Development.json # Development configuration
├── PackPlanner.Tests/              # Unit Tests
│   ├── PackPlanner.Tests.csproj    # Test project configuration
│   ├── PackPlannerTests.cs         # Core algorithm tests
│   ├── ItemTests.cs                # Item struct tests
│   ├── PackTests.cs                # Pack container tests
│   ├── BlockingStrategyTests.cs    # Blocking strategy tests
│   ├── ParallelStrategyTests.cs    # Parallel strategy tests
│   └── PackPlannerStrategyTestBase.cs # Base test class
├── README.md                       # This file
└── PERFORMANCE_COMPARISON.md       # C++ vs C# performance analysis
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

## Web API Architecture

### API Design Principles
- **RESTful Design**: Following REST conventions for predictable API behavior
- **OpenAPI Specification**: Full Swagger/OpenAPI documentation for easy integration
- **Async-First**: Designed for high-concurrency scenarios with async/await patterns
- **Validation-Heavy**: Comprehensive input validation with detailed error responses
- **Rate Limited**: Built-in protection against abuse and resource exhaustion
- **Health Monitoring**: Comprehensive health checks for production deployment

### Rate Limiting
The API implements sophisticated rate limiting:
- **Sync Endpoints**: 100 requests per 15 minutes per IP
- **Async Endpoints**: 50 requests per 15 minutes per IP
- **Info Endpoints**: 200 requests per 15 minutes per IP
- **Headers**: Rate limit information included in response headers

### Async Job Processing
For large datasets, the API supports asynchronous processing:
1. Submit job via `POST /api/packplanner/plan-async`
2. Receive job ID and status URL
3. Poll job status via `GET /api/packplanner/jobs/{jobId}`
4. Retrieve results when completed via `GET /api/packplanner/jobs/{jobId}/result`

### Error Handling
The API provides detailed error responses:
- **400 Bad Request**: Invalid input data with validation details
- **429 Too Many Requests**: Rate limit exceeded
- **404 Not Found**: Job not found or expired
- **500 Internal Server Error**: Unexpected server errors

### Future Enhancements

#### Console Application
1. **Memory Pooling**: ArrayPool<T> for reduced allocations
2. **SIMD Operations**: Vector<T> for mathematical operations
3. **Native AOT**: Ahead-of-time compilation for faster startup
4. **Benchmarking**: Integration with BenchmarkDotNet for detailed performance analysis

#### Web API
1. **Authentication**: JWT-based authentication for enterprise use
2. **Database Integration**: Persistent job storage and history
3. **Caching**: Redis-based caching for frequently requested configurations
4. **Metrics**: Prometheus/Grafana integration for monitoring
5. **Containerization**: Docker support for easy deployment
6. **Load Balancing**: Support for horizontal scaling
7. **WebSocket Support**: Real-time job progress updates
8. **File Upload**: Support for bulk item import via CSV/Excel files
