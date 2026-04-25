@echo off
if "%~1"=="" (
    echo Usage: dds_ktx.bat [folder path]
    exit /b 1
)

for %%f in ("%~1\*.dds") do (
    nvtt_export "%%f" --no-mips --format bc7 --output "%~1\%%~nf.ktx2"
)

echo Done.