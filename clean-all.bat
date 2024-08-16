@echo off
IF NOT EXIST build (
    ECHO "Build directory does not exist"
) ELSE (
    rmdir /S /Q build
    ECHO "Build directory cleaned"
)

PAUSE