@echo off
echo -- Clear and Create .\Intermediate\Build
rmdir /s /q ".\Intermediate\Build"
mkdir ".\Intermediate\Build"

echo -- Generate C++ Files for Shader Parser using Flex/Bison
rmdir /s /q %SHADERLANG_WORKSPACE%\Generated
mkdir %SHADERLANG_WORKSPACE%\Generated

set FLEXBISON_BINARY_DIR=.\Source\ThirdParty\FlexBison
set SHADERLANG_WORKSPACE=.\Source\Runtime\ShaderLang

%FLEXBISON_BINARY_DIR%\win_bison.exe --version
%FLEXBISON_BINARY_DIR%\win_flex.exe --version

%FLEXBISON_BINARY_DIR%\win_bison.exe --yacc -dv ^
    --output=%SHADERLANG_WORKSPACE%\Generated\parser.tab.cpp ^
    --defines=%SHADERLANG_WORKSPACE%\Generated\parser.tab.h ^
    %SHADERLANG_WORKSPACE%\Private\parser.y

%FLEXBISON_BINARY_DIR%\win_flex.exe -d ^
    --outfile=%SHADERLANG_WORKSPACE%\Generated\lexer.tab.cpp ^
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