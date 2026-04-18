## File Logging
The engine will produce log files in the logs/ folder in the root. This log folder can be made more or less verbose via the `doc-error-logging-system.md`.

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
   $env:VULKAN_SDK = [System.Environment]::GetEnvironmentVariable("VULKAN_SDK", "Machine")
   ```

2. Navigate to the project directory and build using MSBuild:
   ```powershell
   $vs = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -property installationPath
   $msbuild = Join-Path $vs "MSBuild\Current\Bin\MSBuild.exe"
   ```

3. **Build** (from the repo root, the folder that contains `voxceleron2.sln`):
   ```powershell
   Set-Location C:\path\to\voxceleron2-engine
   & $msbuild voxceleron2.sln /p:Configuration=Release /p:Platform=x64 /v:m
   ```