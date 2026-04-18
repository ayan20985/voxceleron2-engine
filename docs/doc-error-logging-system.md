# Voxceleron2 Error Logging System

This document describes the comprehensive error logging system integrated into Voxceleron2 for debugging and monitoring engine behavior.

## Overview

The logging system provides thread-safe, multi-level logging with file output and console output capabilities. It includes performance timing utilities, Vulkan API error checking, and memory tracking for debugging engine issues.

## Components

### 1. Logger Class (`Logger.hpp`, `Logger.cpp`)

The core logging system with the following features:
- Thread-safe logging using mutexes
- Multiple log levels (TRACE, DEBUG, INFO, WARNING, ERROR, FATAL)
- File output with automatic directory creation
- Console output with error/warning highlighting
- Timestamps, thread IDs, file/line/function context
- Performance timing utilities

### 2. Logging Macros (`LoggerMacros.hpp`)

Convenient macros that automatically capture file, line, and function information:
- `LOG_TRACE(message)` - Detailed trace information
- `LOG_DEBUG(message)` - Debug information
- `LOG_INFO(message)` - General information
- `LOG_WARNING(message)` - Warning messages
- `LOG_ERROR(message)` - Error messages
- `LOG_FATAL(message)` - Fatal error messages
- `LOG_TIMER(name)` - Performance timing scope
- `VK_CHECK(result, operation)` - Vulkan API result checking

### 3. Memory Tracker (`MemoryTracker.hpp`, `MemoryTracker.cpp`)

Tracks memory allocations and deallocations:
- Atomic counters for thread safety
- Peak memory usage tracking
- Memory leak detection
- Allocation/deallocation logging macros

## Log Levels

- **TRACE**: Very detailed information for low-level debugging
- **DEBUG**: Debug information useful for development
- **INFO**: General information about engine operations
- **WARNING**: Warning messages that don't stop execution
- **ERROR**: Error messages that indicate problems
- **FATAL**: Fatal errors that require engine shutdown

## Usage Examples

### Basic Logging

```cpp
#include "LoggerMacros.hpp"

// Simple logging
LOG_INFO("Engine initialized successfully");
LOG_DEBUG("Player position: " + std::to_string(x) + ", " + std::to_string(y));
LOG_ERROR("Failed to load texture: " + texture_path);
```

### Performance Timing

```cpp
#include "LoggerMacros.hpp"

void load_world() {
    LOG_TIMER("World Loading");
    // World loading code here
    // Timer automatically logs duration on scope exit
}
```

### Vulkan Error Checking

```cpp
#include "LoggerMacros.hpp"

vk::Result result = device.createBuffer(&buffer_info, nullptr, &buffer);
VK_CHECK(result, "create buffer");
// Automatically logs error if result is not eSuccess
```

### Memory Tracking

```cpp
#include "MemoryTracker.hpp"

// Manual tracking
TRACK_ALLOCATION(sizeof(MyClass), "MyClass");

// Or use the RAII wrapper
TrackedPointer<MyClass> ptr(new MyClass(), sizeof(MyClass), "MyClass");
// Automatically tracks allocation and deallocation
```

## Configuration

The logger is initialized in `Core::initialize()` with:
- Log file path: `logs/voxceleron2.log`
- Default level: DEBUG in debug builds, INFO in release builds
- Console output: Disabled in terminal mode

## Log File Format

```
[HH:MM:SS.mmm] [LEVEL] [THREAD_ID] [file:line] [function] message
```

Example:
```
[14:32:15.123] [INFO] [140234567890] [Main.cpp:25] [WinMain] Voxceleron2 Engine initialized
[14:32:15.456] [DEBUG] [140234567891] [Device.cpp:142] [select_gpu] Selected GPU: NVIDIA GeForce RTX 3080 (Rating: 3)
```

## Integrated Systems

### Core Engine
- Initialization and shutdown logging
- Error handling improvements
- Main loop timing information

### Vulkan System
- GPU selection and rating details
- Device creation validation
- Swapchain operations
- API error checking

### World System
- Chunk loading/unloading operations
- Threading performance metrics
- Population and mesh generation timing
- Memory usage statistics

### Rendering System
- Pipeline creation timing
- Framebuffer setup
- Render pass operations
- Resource creation logging

## Debugging Features

### Performance Monitoring
- Automatic timing of critical operations
- Frame-by-frame statistics logging
- Memory usage tracking

### Error Context
- Full stack traces with file/line information
- Thread identification for multi-threaded issues
- Vulkan operation context for API errors

### Memory Leak Detection
- Allocation/deallocation balance checking
- Peak memory usage reporting
- Active allocation counting

## Best Practices

1. **Use appropriate log levels**: TRACE for detailed debugging, INFO for important events, ERROR for problems
2. **Include context**: Log relevant data like positions, counts, states
3. **Use timing scopes**: Wrap performance-critical sections with LOG_TIMER
4. **Check Vulkan results**: Always use VK_CHECK for Vulkan API calls
5. **Memory tracking**: Use TRACK_ALLOCATION/TRACK_DEALLOCATION for custom allocations

## Log Analysis

The log files can be analyzed for:
- Performance bottlenecks (look for long TIMER durations)
- Memory leaks (unbalanced allocations/deallocations)
- Vulkan issues (API errors and validation failures)
- Multi-threading problems (thread-specific error patterns)
- Engine initialization problems (failed resource creation)

## Future Enhancements

The logging system is designed to be extensible for:
- Log filtering and searching
- Remote logging capabilities
- Structured logging (JSON format)
- Performance profiling integration
- Crash dump generation
