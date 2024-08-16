@echo off
REM Build Everything

ECHO "Building everything..."

REM Engine
IF NOT EXIST build (mkdir build)

PUSHD build
cmake -G "MinGW Makefiles" ..
make
POPD
IF %ERRORLEVEL% NEQ 0 (echo Error:%ERRORLEVEL% && PAUSE)

ECHO "All assemblies built successfully."