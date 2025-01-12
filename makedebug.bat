@echo off
setlocal

REM Define the root directory of the project
set "PROJECT_ROOT=%~dp0"

REM Navigate to the project root directory
cd /d "%PROJECT_ROOT%"

REM Define the build directory
set "BUILD_DIR=build_debug"

REM Create the build directory if it doesn't exist
if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
)

REM Navigate to the build directory
cd "%BUILD_DIR%"

REM Run CMake configuration with Ninja, using Clang as the compiler, and set build type to Debug
cmake -G "Ninja" -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake -DCMAKE_BUILD_TYPE=Debug ..

if %errorlevel% neq 0 (
    echo Configuration failed.
    exit /b %errorlevel%
)

REM Build with Ninja, enabling parallel compilation
cmake --build . -- -j %NUMBER_OF_PROCESSORS%
if %errorlevel% neq 0 (
    echo Build failed.
    exit /b %errorlevel%
)

REM Notify success
echo Build completed successfully.