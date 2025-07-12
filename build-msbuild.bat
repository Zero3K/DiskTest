@echo off
echo Building DiskTest with MSBuild...

rem Try to find MSBuild
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe" (
    set MSBUILD="C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" (
    set MSBUILD="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe" (
    set MSBUILD="C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
) else (
    echo MSBuild not found. Please ensure Visual Studio 2019 is installed.
    pause
    exit /b 1
)

echo Using MSBuild: %MSBUILD%
%MSBUILD% DiskTest.sln /p:Configuration=Release /p:Platform=x64

if exist "x64\Release\DiskTest.exe" (
    echo Build completed successfully!
    echo Executable is located at: x64\Release\DiskTest.exe
) else if exist "Release\DiskTest.exe" (
    echo Build completed successfully!
    echo Executable is located at: Release\DiskTest.exe
) else (
    echo Build failed. Please check for errors.
)
pause