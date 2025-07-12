@echo off
echo Building DiskTest with Visual Studio...
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\IDE\devenv.exe" (
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\IDE\devenv.exe" DiskTest.sln /build Release
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\devenv.exe" (
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\devenv.exe" DiskTest.sln /build Release
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\Common7\IDE\devenv.exe" (
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\Common7\IDE\devenv.exe" DiskTest.sln /build Release
) else (
    echo Visual Studio 2019 not found. Please install Visual Studio 2019.
    echo Alternatively, you can open DiskTest.sln manually in Visual Studio.
    pause
    exit /b 1
)

if exist "Release\DiskTest.exe" (
    echo Build completed successfully!
    echo Executable is located at: Release\DiskTest.exe
) else (
    echo Build failed. Please check for errors.
)
pause