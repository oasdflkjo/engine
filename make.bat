@echo off
setlocal

REM Define the build directory
set "BUILD_DIR=build"

REM Create the build directory if it doesn't exist
if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
)

REM Navigate to the build directory
cd "%BUILD_DIR%"

REM Run CMake configuration with Ninja, using Clang as the compiler
cmake -G "Ninja" -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake ..

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
