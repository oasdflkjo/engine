@echo off
setlocal

REM Define the build directory
set "BUILD_DIR=build"

REM Check if the build directory exists
if exist "%BUILD_DIR%" (
    echo Deleting build directory...
    rmdir /S /Q "%BUILD_DIR%"
    echo Build directory deleted.
) else (
    echo No build directory found.
)

REM Remove main.exe if it exists in the project root
if exist "main.exe" (
    echo Deleting main.exe from project root...
    del "main.exe"
    echo main.exe deleted.
) else (
    echo No main.exe found in project root.
)

REM Remove main.pdb if it exists in the project root
if exist "main.pdb" (
    echo Deleting main.pdb from project root...
    del "main.pdb"
    echo main.pdb deleted.
) else (
    echo No main.pdb found in project root.
)

echo Clean completed.
