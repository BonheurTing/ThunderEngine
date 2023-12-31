# minimum Cmake version required
cmake_minimum_required(VERSION 3.24.0)

# project name, dll or exe
project(RomanticEditor VERSION 1.0)

# specify the C++ standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED On)

# cmake include
include(GenerateExportHeader)
include(CMakeUtilities.cmake)

# project output
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Intermediate/Build/Output/Bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Intermediate/Build/Output/Bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Intermediate/Build/Output/Bin)
set(CMAKE_PDB_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Intermediate/Build/Output/Bin)

# CMAKE settings
#SET (CMAKE_CXX_COMPILER_ENV_VAR "clang++")
SET (CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELEASE "-O4 -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
SET (CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Definition
add_definitions(-D_UNICODE)
add_definitions(-D_WINDOWS)
add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
add_definitions(-DNOMINMAX)
add_definitions(-D_USE_MATH_DEFINES)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS" ) 

PreBuildModule()
BuildModule()
PostBuildModule()
MoveMSVCSolutionFile()
file(COPY ${CMAKE_SOURCE_DIR}/Shader/shaders.hlsl DESTINATION ${CMAKE_SOURCE_DIR}/Intermediate/Build/Output/Bin/Debug)