cmake_minimum_required(VERSION 3.10)
project(Voxceleron2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Logger Test Executable
add_executable(logger_test
    src/Oreginum/Logger.cpp
    src/Test/Logger_Test.cpp
)

target_include_directories(logger_test PRIVATE 
    src/
    external/GLM/
)

# Link required libraries for Windows
if(WIN32)
    target_link_libraries(logger_test)
endif()

# Set output directory
set_target_properties(logger_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Optional: Main engine executable (commented out due to dependencies)
# add_executable(voxceleron2
#     src/Tetra/Main.cpp
#     src/Oreginum/Core.cpp
#     src/Oreginum/Logger.cpp
#     src/Tetra/World.cpp
#     # ... other source files
# )