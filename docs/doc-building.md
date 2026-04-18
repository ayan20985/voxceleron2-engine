## File Logging
The engine will produce log files in the logs/ folder in the root. This log folder can be made more or less verbose via the doc-error-logging-system.md.

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

### Building from Command Line
You can also build the project from the command line:

1. Launch the Visual Studio Developer PowerShell:
   ```powershell
   & 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1'
   ```

2. Navigate to the project directory and build using MSBuild:
   ```powershell
   cd C:\path\to\voxceleron2-engine
   MSBuild.exe voxceleron2.sln /p:Configuration=Release /p:Platform=x64
   ```