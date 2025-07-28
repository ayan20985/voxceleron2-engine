# Voxceleron2 Engine

A voxel-based rendering engine built with C++ and Vulkan.

![Screenshot 2025-03-19 163843](https://github.com/user-attachments/assets/b88903c6-2c41-4211-b291-d7fd4041f25b)
![Screenshot 2025-03-19 185421](https://github.com/user-attachments/assets/57cd0df5-5ec6-4d08-839d-34a721c4816b)
![Screenshot 2025-03-19 235115](https://github.com/user-attachments/assets/ede37aef-b082-4d33-88e3-8d24147b1569)

# Current progress

Infinite world are working, gone is the blasted array-based storage of the world and a Sparse Octree has been introduced :) Beggining LOD development now.

<img width="1291" height="731" alt="Screenshot 2025-06-08 203114" src="https://github.com/user-attachments/assets/e32f4dc1-9c7c-48c8-a982-694d2847f83d" />
<img width="1295" height="731" alt="Screenshot 2025-06-08 203214" src="https://github.com/user-attachments/assets/c121957e-0ba4-4dcd-9810-2b76f4aefae6" />
<img width="1285" height="729" alt="Screenshot 2025-06-08 205259" src="https://github.com/user-attachments/assets/6bc6151a-6fe1-40db-baaa-ea356244d5c3" />
<img width="1285" height="726" alt="Screenshot 2025-06-08 213108" src="https://github.com/user-attachments/assets/d0b3ad80-9d4f-4664-b4ad-cb3ef4781bab" />
<img width="1919" height="1079" alt="Screenshot 2025-06-10 214218" src="https://github.com/user-attachments/assets/ca10afe7-ad9a-4440-9767-adc591c10626" />

LOD Development is not going to plan.

<img width="1919" height="1079" alt="Screenshot 2025-06-10 191525" src="https://github.com/user-attachments/assets/140ea5e5-d398-40b2-9866-a00fef769271" />
<img width="1919" height="1079" alt="Screenshot 2025-06-10 191509" src="https://github.com/user-attachments/assets/f26592cf-2e95-469d-80ff-7b9ef2ca4cac" />

# Project Details
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
