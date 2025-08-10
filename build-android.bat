@echo off
REM Build OpenCLI for Android using NDK or MinGW

REM Check cmake
where cmake >nul 2>nul
if errorlevel 1 (
    echo Error: cmake not found in PATH
    exit /b 1
)

REM Build MinGW
echo Building MinGW...
if exist build-mingw rmdir /s /q build-mingw
mkdir build-mingw
cd build-mingw

cmake .. -G "MinGW Makefiles" ^
    -DCMAKE_C_COMPILER=gcc ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DANDROID=ON

mingw32-make
if exist opencli.exe ren opencli.exe opencli
cd ..

REM Build ARM64
echo Building ARM64...
if exist build-arm64 rmdir /s /q build-arm64
mkdir build-arm64
cd build-arm64

cmake .. -G "MinGW Makefiles" ^
    -DCMAKE_TOOLCHAIN_FILE=D:/android-ndk-r27d/build/cmake/android.toolchain.cmake ^
    -DANDROID_NDK=D:/android-ndk-r27d ^
    -DANDROID_ABI=arm64-v8a ^
    -DANDROID_PLATFORM=android-29 ^
    -DANDROID_STL=c++_static ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_C_FLAGS="-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0"

mingw32-make
if exist opencli.exe ren opencli.exe opencli
cd ..

REM Build ARM32
echo Building ARM32...
if exist build-arm32 rmdir /s /q build-arm32
mkdir build-arm32
cd build-arm32

cmake .. -G "MinGW Makefiles" ^
    -DCMAKE_TOOLCHAIN_FILE=D:/android-ndk-r27d/build/cmake/android.toolchain.cmake ^
    -DANDROID_NDK=D:/android-ndk-r27d ^
    -DANDROID_ABI=armeabi-v7a ^
    -DANDROID_PLATFORM=android-29 ^
    -DANDROID_STL=c++_static ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_C_FLAGS="-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0"

mingw32-make
cd ..

echo Build complete: MinGW, ARM64, ARM32
