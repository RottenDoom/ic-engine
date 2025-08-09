@echo off
REM Usage:
REM build-all.bat <compiler> <build-type>
REM Example: build-all.bat gcc debug

IF "%~1"=="" (
    echo No compiler specified. Use gcc or clang.
    exit /b 1
)
IF "%~2"=="" (
    echo No build type specified. Use debug or release.
    exit /b 1
)

set COMPILER=%~1
set BUILDTYPE=%~2

ECHO Building with %COMPILER% in %BUILDTYPE% mode...

IF /I "%COMPILER%"=="gcc" (
    set PRESET=gcc-%BUILDTYPE%
    IF NOT EXIST build\build-gcc (mkdir build\build-gcc)
) ELSE IF /I "%COMPILER%"=="clang" (
    set PRESET=clang-%BUILDTYPE%
    IF NOT EXIST build\build-clang (mkdir build\build-clang)
) ELSE (
    echo Invalid compiler: %COMPILER%
    exit /b 1
)

REM Run cmake commands from project root, not from build directory
cmake --preset=%PRESET%
IF %ERRORLEVEL% NEQ 0 (
    echo Configure failed with error code %ERRORLEVEL%.
    exit /b %ERRORLEVEL%
)

cmake --build --preset=%PRESET%
IF %ERRORLEVEL% NEQ 0 (
    echo Build failed with error code %ERRORLEVEL%.
    exit /b %ERRORLEVEL%
)

ECHO Build complete: %COMPILER% (%BUILDTYPE%)