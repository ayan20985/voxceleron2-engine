# Voxceleron2 Logger Architecture

## Overview
Simple console-based logging system for the Voxceleron2 engine to aid in debugging during infinite terrain generation development.

## Design Goals
- **Simplicity**: Easy to integrate and use throughout the codebase
- **Performance**: Minimal overhead for production builds
- **Readability**: Clear, consistent console output format
- **Integration**: Works seamlessly with existing Oreginum::Core architecture

## Log Levels
- `INFO`: General information about engine state and operations
- `WARN`: Warnings that don't stop execution but indicate potential issues  
- `EXCEP`: Exceptions and critical errors that may cause instability

## Architecture

### Core Components

#### Logger Class (`src/Oreginum/Logger.hpp/.cpp`)
```cpp
namespace Oreginum {
    class Logger {
    public:
        enum class Level { INFO, WARN, EXCEP };
        
        static void log(Level level, const std::string& message);
        static void info(const std::string& message);
        static void warn(const std::string& message);
        static void excep(const std::string& message);
        
        static void set_enabled(bool enabled);
        static bool is_enabled();
    };
}
```

#### Integration Points
1. **Core Integration**: Add to [`Oreginum::Core::initialize()`](src/Oreginum/Core.cpp:25) 
2. **Error Replacement**: Enhance existing [`error()`](src/Oreginum/Core.cpp:50) function
3. **World System**: Add logging to [`Tetra::World`](src/Tetra/World.hpp) for terrain generation
4. **Vulkan Operations**: Log critical GPU operations and resource management

## Output Format
```
[TIMESTAMP] [LEVEL] MESSAGE
[12:34:56] [INFO ] Engine initialized successfully
[12:34:57] [WARN ] Low memory warning: 85% usage
[12:34:58] [EXCEP] Failed to allocate chunk buffer
```

## Usage Examples

### Basic Logging
```cpp
#include "../Oreginum/Logger.hpp"

// Simple logging
Oreginum::Logger::info("Chunk loaded at position (16, 0, 16)");
Oreginum::Logger::warn("Texture memory usage exceeding threshold");
Oreginum::Logger::excep("Vulkan buffer allocation failed");

// Formatted logging
Oreginum::Logger::info("Generated " + std::to_string(count) + " vertices");
```

### Integration with Existing Systems
```cpp
// Replace current error handling
void Oreginum::Core::error(const std::string& error) {
    Logger::excep("FATAL: " + error);
    destroy();
    MessageBox(NULL, error.c_str(), "Oreginum Engine Error", MB_ICONERROR);
    std::exit(EXIT_FAILURE);
}
```

## Implementation Strategy

### Phase 1: Core Logger Creation
1. Create [`Logger.hpp`](src/Oreginum/Logger.hpp) and [`Logger.cpp`](src/Oreginum/Logger.cpp)
2. Implement thread-safe console output
3. Add timestamp formatting
4. Integrate with [`Core::initialize()`](src/Oreginum/Core.cpp:25)

### Phase 2: Strategic Integration  
1. Update [`Core::error()`](src/Oreginum/Core.cpp:50) to use logger
2. Add logging to main loop and key initialization points
3. Add world generation logging hooks

### Phase 3: Comprehensive Coverage
1. Add Vulkan operation logging
2. Add performance monitoring logs
3. Add memory management logging
4. Document logging conventions

## Technical Details

### Thread Safety
- Use mutex protection for console output
- Atomic operations for enable/disable state
- No dynamic memory allocation in critical paths

### Performance Considerations
- Compile-time level filtering for release builds
- String formatting only when logging is enabled
- Minimal stack overhead per call

### Platform Compatibility
- Windows console output using WriteConsole API
- ANSI color codes for level differentiation
- Unicode support for international characters

## Future Enhancements (Post-Implementation)
- File output option
- Network logging for remote debugging  
- Performance metrics integration
- Log filtering by subsystem
- Configuration file support