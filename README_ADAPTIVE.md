# Pack Planner - Adaptive Processing System

This document describes the new unified Pack Planner system that intelligently chooses between client-side WASM processing and server-side Web API processing based on runtime system profiling.

## Overview

The Pack Planner now features an adaptive processing system that automatically determines the optimal processing method based on:

- **CPU Performance**: Measured using WASM-based computational benchmarks
- **Memory Bandwidth**: System memory performance testing
- **Network Latency**: Real-time network latency measurements
- **Network Bandwidth**: Bandwidth testing with external endpoints
- **Battery Status**: Device battery level and charging status (when available)

## Architecture

### Files Structure

```
pack_planner/
├── index.html          # Unified HTML interface
├── styles.css          # Consolidated CSS styles
├── app.js              # Main application logic with adaptive processing
├── CMakeLists.txt       # Updated build configuration
├── index_rest.html      # Legacy REST API interface (reference)
├── wasm_profiler.html   # Legacy profiler interface (reference)
└── README_ADAPTIVE.md   # This documentation
```

### Key Components

1. **SystemProfiler Class**: Performs comprehensive system profiling
2. **ProcessingModeManager**: Determines optimal processing mode
3. **WASMLoader**: Handles WASM module loading and initialization
4. **APIClient**: Manages Web API communication
5. **WASMClient**: Handles client-side WASM processing

## Processing Modes

### Auto Mode (Default)
- Automatically profiles the system
- Makes intelligent decisions based on weighted scoring:
  - CPU Score (30%)
  - Memory Bandwidth (25%)
  - Network Latency (20%)
  - Network Bandwidth (15%)
  - Battery Level (10%)

### Manual Override Modes
- **Force WASM**: Always use client-side processing
- **Force Web API**: Always use server-side processing

## Decision Algorithm

The system calculates a confidence score based on normalized metrics:

```javascript
overallScore = 
    cpuNorm * 0.3 +
    memoryNorm * 0.25 +
    latencyNorm * 0.2 +
    bandwidthNorm * 0.15 +
    batteryNorm * 0.1
```

**Decision Thresholds:**
- Score > 0.7: Prefer WASM (client-side)
- Score 0.4-0.7: Hybrid (prefer WASM if available, fallback to API)
- Score < 0.4: Prefer Web API (server-side)

## Features

### Unified Interface
- Single HTML file with tabbed interface
- Real-time processing mode display
- System profiling results visualization
- Manual mode override controls

### Adaptive Benchmarking
- Benchmarks automatically use the determined optimal processing mode
- Results include processing mode information
- CSV export includes processing mode data

### Async Processing Support
- Automatic async mode for large datasets when using Web API
- Job status monitoring and cancellation
- Progress tracking and ETA estimation

### Error Handling & Fallbacks
- Graceful degradation when WASM is unavailable
- Network error handling with fallbacks
- Comprehensive error reporting

## Usage

### Basic Operation
1. Open `index.html` in a web browser
2. The system automatically profiles your environment
3. Choose processing mode (Auto recommended)
4. Configure packing parameters
5. Generate or input item data
6. Run packing operation

### Manual Mode Selection
- **Auto Mode**: Let the system decide (recommended)
- **Force WASM**: Use for testing client-side performance
- **Force API**: Use for testing server-side performance or when WASM is unavailable

### Benchmarking
1. Switch to "Performance Benchmark" tab
2. Configure benchmark parameters
3. Run single benchmark or full benchmark suite
4. Export results to CSV for analysis

## Building

### WASM Build
```bash
mkdir build-wasm
cd build-wasm
emcmake cmake ..
emmake make
```

The build system automatically copies all necessary files (`index.html`, `styles.css`, `app.js`, etc.) to the build directory.

### Native Build
```bash
mkdir build
cd build
cmake ..
make
```

## API Compatibility

The system maintains full compatibility with the existing REST API:

- **Sync Endpoint**: `POST /api/pack`
- **Async Endpoint**: `POST /api/pack/async`
- **Status Endpoint**: `GET /api/pack/status/{jobId}`
- **Result Endpoint**: `GET /api/pack/result/{jobId}`
- **Benchmark Endpoint**: `POST /api/benchmark`
- **Health Endpoint**: `GET /api/health`

## Configuration

### API Configuration
- Base URL: Configurable in the UI (default: `http://localhost:5135/api`)
- Timeout: 5 minutes for large operations
- Retry attempts: 3 automatic retries

### Profiling Configuration
- Network test endpoints: Multiple public APIs for latency testing
- Bandwidth test: 256KB payload for quick results
- CPU/Memory tests: WASM-based computational benchmarks

## Browser Compatibility

- **Modern Browsers**: Full functionality with WASM and all APIs
- **Legacy Browsers**: Automatic fallback to Web API mode
- **Mobile Devices**: Optimized for battery-aware processing decisions

## Performance Considerations

### Client-side (WASM) Advantages
- No network latency for processing
- Utilizes local CPU resources
- Better for smaller datasets
- Privacy-friendly (data stays local)

### Server-side (API) Advantages
- Consistent performance regardless of client hardware
- Better for very large datasets
- Reduces client battery usage
- Centralized processing power

## Troubleshooting

### WASM Loading Issues
- Ensure files are served from HTTP/HTTPS (not file://)
- Check browser console for detailed error messages
- Verify WASM files are in the same directory

### API Connection Issues
- Verify API server is running
- Check CORS configuration
- Confirm API base URL is correct
- Test network connectivity

### Performance Issues
- Monitor system profiling results
- Consider manual mode override for testing
- Check browser developer tools for bottlenecks

## Future Enhancements

- Machine learning-based decision optimization
- Historical performance tracking
- Advanced caching strategies
- Progressive Web App (PWA) support
- WebWorker integration for better UI responsiveness
