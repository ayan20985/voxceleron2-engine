@echo off
echo Voxceleron2 Engine Build Script
echo =============================

REM Check if Vulkan SDK is installed
if "%VULKAN_SDK%"=="" (
    echo Error: Vulkan SDK not found. Please install Vulkan SDK and set the VULKAN_SDK environment variable.
    exit /b 1
)

REM Create build directory if it doesn't exist
if not exist "build" mkdir build

REM Compile shaders
echo Compiling shaders...
if not exist "shader_bin" mkdir shader_bin

REM Compile all vertex shaders
for %%f in (shaders\*.vert) do (
    echo Compiling vertex shader: %%f
    "%VULKAN_SDK%\Bin\glslc.exe" %%f -o shader_bin\%%~nf.spv
)

REM Compile all fragment shaders
for %%f in (shaders\*.frag) do (
    echo Compiling fragment shader: %%f
    "%VULKAN_SDK%\Bin\glslc.exe" %%f -o shader_bin\%%~nf.spv
)

REM Navigate to build directory and run CMake
cd build
echo Configuring CMake...

REM Initialize Visual Studio environment and run CMake
echo Generating project files with CMake...
cmake -G "Visual Studio 17 2022" -A x64 ..

REM Build the project
echo Building project...
cmake --build . --config Release

REM Check if build was successful
if %ERRORLEVEL% NEQ 0 (
    echo Build failed.
    exit /b 1
)

REM Copy shader files
echo Copying shader files...
if not exist "Release\shaders" mkdir Release\shaders
xcopy /y /i ..\shader_bin\*.spv Release\shaders\

echo Build completed successfully.
echo Executable located at: build\Release\voxceleron2.exe 