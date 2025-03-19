# Voxceleron2 Engine

A voxel-based rendering engine built with C++ and Vulkan.

![Voxceleron2 Engine](https://example.com/screenshot.png)

## Features

- Procedurally generated voxel terrain
- Advanced lighting with ambient occlusion
- Bloom and post-processing effects
- Real-time shadows
- Translucent materials
- Optimized chunk-based rendering

## Dependencies

The following dependencies are required to build the engine:

- **Visual Studio 2022** or newer
- **Vulkan SDK 1.4.0** or newer - [Download from LunarG](https://vulkan.lunarg.com/sdk/home)
- **GLM** (included in the `external` directory)
- **STB_IMAGE** (included in the `external` directory)
- **FastNoiseSIMD** (included in the `external` directory)

## Build Instructions

### Setting up the environment

1. Install [Visual Studio 2022](https://visualstudio.microsoft.com/vs/) with C++ development workload
2. Install the [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)
3. Ensure that the `VULKAN_SDK` environment variable is set correctly (typically done by the installer)

### Building the project

1. Open the solution file `voxceleron2.sln` in Visual Studio
2. Select the desired configuration (Debug or Release) and platform (x64 recommended)
3. Build the solution (F7 or Build → Build Solution)
4. The executable will be created in `build\x64\[Configuration]\voxceleron2.exe`

### Running the engine

Run the generated executable from Visual Studio or directly from the build directory. Resources will be automatically copied to the output directory.

## Controls

- **W, A, S, D**: Move forward, left, backward, right
- **Mouse**: Look around
- **Shift**: Run faster
- **L**: Unlock Mouse

## Troubleshooting

If you encounter build errors:

1. Make sure the Vulkan SDK is properly installed and the `VULKAN_SDK` environment variable is set
2. Check Visual Studio has the C++ desktop development workload installed
3. Verify that all required libraries are in their expected locations

## License

This project is provided as-is with no warranty. See the license file for details.
