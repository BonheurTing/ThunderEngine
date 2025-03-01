# minimum Cmake version required
cmake_minimum_required(VERSION 3.24.0)

# project name, dll or exe
project(ThunderEditor VERSION 1.0)

# specify the C++ standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
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
SET(CMAKE_BUILD_TYPE "RELWITHDEBINFO")
#SET (CMAKE_CXX_COMPILER_ENV_VAR "clang++")
SET (CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELEASE "-O4 -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -Zi")
SET (CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Definition
add_definitions(-D_UNICODE)
add_definitions(-D_WINDOWS)
add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
add_definitions(-DNOMINMAX)
add_definitions(-D_USE_MATH_DEFINES)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS" )
add_definitions(-DTRACY_ENABLE)

PreBuildModule()
BuildModule()
PostBuildModule()
#MoveMSVCSolutionFile()
file(COPY ${CMAKE_SOURCE_DIR}/Shader/shaders.hlsl DESTINATION ${CMAKE_BINARY_DIR}/Output/Bin/Debug)
file(COPY ${CMAKE_SOURCE_DIR}/Shader/shaders.hlsl DESTINATION ${CMAKE_BINARY_DIR}/Output/Bin/RelWithDebInfo)
file(COPY ${CMAKE_SOURCE_DIR}/Source/ThirdParty/Windows/DXCompiler/bin/dxcompiler.dll DESTINATION ${CMAKE_BINARY_DIR}/Output/Bin/RelWithDebInfo)

# find_program(Python_EXECUTABLE python NO_CMAKE_FIND_ROOT_PATH)
# set(Python_ARGS "../../Setup.py")
# execute_process(
#   COMMAND ${Python_EXECUTABLE} ${Python_ARGS}
# )