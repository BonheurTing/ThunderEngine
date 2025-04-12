@echo off
echo -- Clear and Create .\Intermediate\Build
rmdir /s /q ".\Intermediate\Build"
mkdir ".\Intermediate\Build"

echo -- Generate C++ Files for Shader Parser using Flex/Bison
set FLEXBISON_BINARY_DIR=.\Source\ThirdParty\FlexBison
rem set BISON_PKGDATADIR=%FLEXBISON_BINARY_DIR%\..\data
rem set M4=%FLEXBISON_BINARY_DIR%\m4.exe

set SHADERLANG_WORKSPACE=.\Source\Runtime\ShaderLang
mkdir %SHADERLANG_WORKSPACE%\Generated

%FLEXBISON_BINARY_DIR%\win_bison.exe --language=c++ -dv ^
    --output=%SHADERLANG_WORKSPACE%\Generated\parser.cpp ^
    --defines=%SHADERLANG_WORKSPACE%\Generated\parser.hpp ^
    %SHADERLANG_WORKSPACE%\Private\parser.y

%FLEXBISON_BINARY_DIR%\win_flex.exe --c++ ^
    -o%SHADERLANG_WORKSPACE%\Generated\lexer.cpp ^
    %SHADERLANG_WORKSPACE%\Private\lexer.l

echo -- Call cmake
cd ./Intermediate/Build
cmake -A x64 ../..
echo Config finished...

rem pause
rem cmake --build .
rem echo Build finished...
rem pause
rem cmake --install . --config Debug
rem echo Install finished...
rem pause