@echo off
REM Build OpenCLI for Android using MSVC and Visual Studio Android NDK

REM Check cmake
where cmake >nul 2>nul
if errorlevel 1 (
    echo Error: cmake not found in PATH
    exit /b 1
)

REM Detect Android NDK
set "VS_NDK_PATH="
if exist "C:\Microsoft\AndroidNDK64\" (
    for /f "delims=" %%i in ('dir /b /ad "C:\Microsoft\AndroidNDK64\android-ndk-r*" 2^>nul') do set "VS_NDK_PATH=C:\Microsoft\AndroidNDK64\%%i"
)
if "%VS_NDK_PATH%"=="" (
    if exist "D:\android-ndk-r27d" (
        set "VS_NDK_PATH=D:\android-ndk-r27d"
    ) else (
        echo Error: Android NDK not found. Please install Android NDK or set path manually.
        echo Expected locations:
        echo   - C:\Microsoft\AndroidNDK64\android-ndk-r*
        echo   - D:\android-ndk-r27d
        exit /b 1
    )
)
echo Using Android NDK: %VS_NDK_PATH%

REM Build ARM64 (Android)
echo Building ARM64 (Android)...
if exist build-arm64 rmdir /s /q build-arm64
mkdir build-arm64
cd build-arm64

cmake .. -G "Ninja" ^
    -DCMAKE_TOOLCHAIN_FILE="%VS_NDK_PATH%/build/cmake/android.toolchain.cmake" ^
    -DANDROID_NDK="%VS_NDK_PATH%" ^
    -DANDROID_ABI=arm64-v8a ^
    -DANDROID_PLATFORM=android-29 ^
    -DANDROID_STL=c++_static ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_C_FLAGS="-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0" ^
    -DCMAKE_MAKE_PROGRAM=ninja

ninja
cd ..

REM Build ARM32 (Android)
echo Building ARM32 (Android)...
if exist build-arm32 rmdir /s /q build-arm32
mkdir build-arm32
cd build-arm32

cmake .. -G "Ninja" ^
    -DCMAKE_TOOLCHAIN_FILE="%VS_NDK_PATH%/build/cmake/android.toolchain.cmake" ^
    -DANDROID_NDK="%VS_NDK_PATH%" ^
    -DANDROID_ABI=armeabi-v7a ^
    -DANDROID_PLATFORM=android-29 ^
    -DANDROID_STL=c++_static ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_C_FLAGS="-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0" ^
    -DCMAKE_MAKE_PROGRAM=ninja

ninja
cd ..

echo Build complete: ARM64 (Android), ARM32 (Android)
echo.
echo Built binaries:
if exist build-arm64\opencli echo   - Android ARM64: build-arm64\opencli
if exist build-arm32\opencli echo   - Android ARM32: build-arm32\opencli
