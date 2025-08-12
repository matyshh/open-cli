cmake_minimum_required(VERSION 3.10)
project(opencli VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/D_CRT_SECURE_NO_WARNINGS /wd4996)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL")
    if(DEFINED ENV{WindowsSdkDir} AND DEFINED ENV{WindowsSDKVersion})
        include_directories("$ENV{WindowsSdkDir}Include/$ENV{WindowsSDKVersion}ucrt")
        include_directories("$ENV{WindowsSdkDir}Include/$ENV{WindowsSDKVersion}um")
        include_directories("$ENV{WindowsSdkDir}Include/$ENV{WindowsSDKVersion}shared")
    else()
        message(WARNING "Windows SDK environment variables not set. Falling back to hardcoded paths.")
        include_directories("C:/Program Files (x86)/Windows Kits/10/Include/10.0.22621.0/ucrt")
        include_directories("C:/Program Files (x86)/Windows Kits/10/Include/10.0.22621.0/um")
        include_directories("C:/Program Files (x86)/Windows Kits/10/Include/10.0.22621.0/shared")
    endif()
    if(DEFINED ENV{VCToolsInstallDir})
        include_directories("$ENV{VCToolsInstallDir}include")
    else()
        message(WARNING "VCToolsInstallDir not set. Falling back to hardcoded path.")
        include_directories("D:/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.44.35207/include")
    endif()
endif()

# Android NDK support
if(ANDROID)
    message(STATUS "Building for Android")
    add_definitions(-D__ANDROID__)
    
    
    if(NOT ANDROID_PLATFORM)
        set(ANDROID_PLATFORM android-21)
    endif()
    
   
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    
    message(STATUS "Android ABI: ${ANDROID_ABI}")
    message(STATUS "Android Platform: ${ANDROID_PLATFORM}")
endif()

add_library(tomlc99 STATIC lib/tomlc99/toml.c)
target_include_directories(tomlc99 PUBLIC lib/tomlc99)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/tomlc99)

add_executable(opencli 
    src/main.c
    src/commands/run_command.c
    src/commands/build_command.c
    src/commands/install_command.c
    src/commands/setup_command.c
    src/utils/process_utils.c
    src/utils/download_utils.c
    src/utils/compiler_utils.c
    src/utils/toml_utils.c
    src/utils/include_utils.c
    src/utils/console_utils.c
)

target_link_libraries(opencli tomlc99)



if(WIN32)
    target_link_libraries(opencli wininet)
elseif(ANDROID)
    target_link_libraries(opencli log)
else()
    find_package(CURL REQUIRED)
    include_directories(${CURL_INCLUDE_DIRS})
    target_link_libraries(opencli ${CURL_LIBRARIES})
endif()

install(TARGETS opencli DESTINATION bin)
install(FILES 
    ${CMAKE_SOURCE_DIR}/compilers.toml
    ${CMAKE_SOURCE_DIR}/opencli.toml
    DESTINATION share/opencli)