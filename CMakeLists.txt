cmake_minimum_required(VERSION 3.10)
project(voxceleron2)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable GLM experimental features
add_definitions(-DGLM_ENABLE_EXPERIMENTAL)

# Find Vulkan
find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${Vulkan_INCLUDE_DIRS}
)

# Source files
file(GLOB_RECURSE VULKAN_SOURCES "${CMAKE_SOURCE_DIR}/src/Vulkan/*.cpp")
file(GLOB_RECURSE OREGINUM_SOURCES "${CMAKE_SOURCE_DIR}/src/Oreginum/*.cpp")
file(GLOB_RECURSE TETRA_SOURCES "${CMAKE_SOURCE_DIR}/src/Tetra/*.cpp")

# Create executable
add_executable(voxceleron2 
    ${VULKAN_SOURCES}
    ${OREGINUM_SOURCES}
    ${TETRA_SOURCES}
)

# Link with Vulkan
target_link_libraries(voxceleron2 PRIVATE Vulkan::Vulkan)

# Windows-specific settings
if(WIN32)
    # Link with needed Windows libraries
    target_link_libraries(voxceleron2 PRIVATE winmm)
    
    # Set subsystem to Windows for GUI application
    set_target_properties(voxceleron2 PROPERTIES
        WIN32_EXECUTABLE TRUE
    )

    # Copy shaders to build directory
    add_custom_command(TARGET voxceleron2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/Release/shaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shader_bin ${CMAKE_BINARY_DIR}/Release/shaders
        COMMENT "Copying shaders to build directory"
    )
    
    # Copy textures to build directory
    add_custom_command(TARGET voxceleron2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/Release/textures
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/textures ${CMAKE_BINARY_DIR}/Release/textures
        COMMENT "Copying textures to build directory"
    )
endif() 