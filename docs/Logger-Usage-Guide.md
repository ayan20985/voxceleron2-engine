# Voxceleron2 Logger Usage Guide

## Quick Start

The logger is now integrated into your Voxceleron2 engine and ready to use throughout your codebase.

### Basic Usage

```cpp
#include "../Oreginum/Logger.hpp"

// Simple logging
Oreginum::Logger::info("Engine started successfully");
Oreginum::Logger::warn("Memory usage is high");
Oreginum::Logger::excep("Critical error occurred");
```

### Formatted Messages

```cpp
// Using string concatenation
int chunk_count = 42;
Oreginum::Logger::info("Generated " + std::to_string(chunk_count) + " chunks");

// Position logging
glm::vec3 pos = {16.0f, 0.0f, 16.0f};
Oreginum::Logger::info("Player position: (" + 
    std::to_string(pos.x) + ", " + 
    std::to_string(pos.y) + ", " + 
    std::to_string(pos.z) + ")");

// Performance metrics
float frame_time = 16.67f;
Oreginum::Logger::warn("Frame time: " + std::to_string(frame_time) + "ms (target: 16.67ms)");
```

## Integration Points

### Already Integrated

1. **Core Engine**: [`Core::initialize()`](../src/Oreginum/Core.cpp) now logs initialization steps
2. **Error Handling**: [`Core::error()`](../src/Oreginum/Core.cpp) logs fatal errors before displaying MessageBox
3. **World System**: [`World::World()`](../src/Tetra/World.cpp) logs world creation and chunk allocation
4. **World Movement**: [`World::move()`](../src/Tetra/World.cpp) logs world shifting operations

### Recommended Integration Points

#### Vulkan Operations
```cpp
// Buffer allocation
Oreginum::Logger::info("Allocating vertex buffer: " + std::to_string(size) + " bytes");
if (result != VK_SUCCESS) {
    Oreginum::Logger::excep("Failed to allocate Vulkan buffer: " + std::to_string(result));
}

// Pipeline creation
Oreginum::Logger::info("Creating graphics pipeline: " + pipeline_name);
```

#### Chunk Management
```cpp
// Chunk generation
Oreginum::Logger::info("Generating chunk at (" + 
    std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")");

// Mesh generation
Oreginum::Logger::info("Generated mesh with " + std::to_string(vertex_count) + " vertices");

// Chunk loading/unloading
Oreginum::Logger::info("Loaded chunk " + std::to_string(chunk_id) + " in " + 
    std::to_string(load_time) + "ms");
```

#### Resource Management
```cpp
// Texture loading
Oreginum::Logger::info("Loading texture: " + filename);
if (!texture_loaded) {
    Oreginum::Logger::warn("Failed to load texture: " + filename + ", using fallback");
}

// Memory monitoring
if (memory_usage > 0.8f) {
    Oreginum::Logger::warn("High memory usage: " + 
        std::to_string(memory_usage * 100) + "%");
}
```

#### Performance Monitoring
```cpp
// Frame rate monitoring
if (fps < 30) {
    Oreginum::Logger::warn("Low FPS detected: " + std::to_string(fps) + " fps");
}

// Generation timing
auto start = std::chrono::high_resolution_clock::now();
// ... terrain generation code ...
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
Oreginum::Logger::info("Terrain generation took " + std::to_string(duration.count()) + "ms");
```

## Logger Control

### Enable/Disable Logging
```cpp
// Disable logging for performance-critical sections
Oreginum::Logger::set_enabled(false);
// ... critical code ...
Oreginum::Logger::set_enabled(true);

// Check if logging is enabled
if (Oreginum::Logger::is_enabled()) {
    // Expensive string formatting only if logging is on
    std::string complex_message = build_complex_debug_info();
    Oreginum::Logger::info(complex_message);
}
```

## Testing the Logger

### Build and Run Test
```powershell
# Run the PowerShell build script
./build_test.ps1
```

### Manual Build
```powershell
# Create build directory
mkdir build
cd build

# Generate build files
cmake .. -G "Visual Studio 17 2022" -A x64

# Build project
cmake --build . --config Release

# Run test
./bin/Release/logger_test.exe
```

### Expected Output
```
=== Voxceleron2 Logger Test ===
Testing basic logging functionality...

[12:34:56] [INFO ] Logger test started
[12:34:56] [INFO ] This is an informational message
[12:34:56] [WARN ] This is a warning message - low memory detected
[12:34:56] [INFO ] Generating terrain chunk at position (16, 0, 16)
[12:34:56] [WARN ] Texture memory usage: 85% - consider optimization
[12:34:56] [EXCEP] Failed to allocate Vulkan buffer - retrying...
[12:34:56] [INFO ] Buffer allocation successful on retry

Testing enable/disable functionality...
[12:34:56] [INFO ] This message should appear
[12:34:56] [INFO ] Logger re-enabled - this message should appear
[12:34:56] [INFO ] Generated 42 chunks
[12:34:56] [WARN ] Memory usage: 67.5%

=== Logger Test Complete ===
```

## Best Practices

### When to Use Each Level

#### INFO
- Engine initialization steps
- Successful operations
- State changes
- Performance metrics
- Resource loading confirmations

#### WARN
- Performance degradation
- Resource usage warnings
- Recoverable errors
- Deprecated functionality usage
- Configuration issues

#### EXCEP
- Fatal errors
- Resource allocation failures
- Vulkan operation failures
- Critical system errors
- Unrecoverable states

### Performance Considerations

1. **Expensive Operations**: Only perform expensive string formatting when logging is enabled
2. **Hot Paths**: Consider disabling logging in performance-critical loops
3. **String Building**: Use efficient string concatenation methods
4. **Conditional Logging**: Check `is_enabled()` before complex message construction

### Message Format Guidelines

1. **Be Descriptive**: Include relevant context and values
2. **Use Consistent Format**: Follow established patterns for similar operations
3. **Include Units**: Specify units for measurements (ms, MB, fps, etc.)
4. **Provide Context**: Include relevant identifiers (chunk coordinates, object names, etc.)

## Thread Safety

The logger is fully thread-safe and can be used from multiple threads simultaneously without additional synchronization.

## Next Steps

1. **Populate Existing Code**: Add logging throughout your existing engine systems
2. **Monitor Performance**: Use logging to identify bottlenecks during terrain generation
3. **Debug Issues**: Use logging to trace execution flow and identify problems
4. **Extend Functionality**: Consider adding file output or network logging in the future