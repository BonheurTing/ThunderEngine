@echo off
set FLEXBISON_BINARY_DIR=.\Source\ThirdParty\FlexBison
set SHADERLANG_WORKSPACE=.\Source\Runtime\ShaderLang

echo -- Generate C++ Files for Shader Parser using Flex/Bison
rmdir /s /q %SHADERLANG_WORKSPACE%\Generated
mkdir %SHADERLANG_WORKSPACE%\Generated

echo -- Check the Flex/Bison version
%FLEXBISON_BINARY_DIR%\win_bison.exe --version
%FLEXBISON_BINARY_DIR%\win_flex.exe --version

echo.
echo -- Begin to generate the parser
%FLEXBISON_BINARY_DIR%\win_bison.exe --yacc -Wno-yacc -Wno-other -dv ^
    --output=%SHADERLANG_WORKSPACE%\Generated\parser.tab.cpp ^
    --defines=%SHADERLANG_WORKSPACE%\Generated\parser.tab.h ^
    %SHADERLANG_WORKSPACE%\Private\parser.y

%FLEXBISON_BINARY_DIR%\win_flex.exe -d ^
    --outfile=%SHADERLANG_WORKSPACE%\Generated\lexer.tab.cpp ^
    %SHADERLANG_WORKSPACE%\Private\lexer.l
echo -- End to generate the parser
echo.

python .\Tools\ShaderLang\GenerateShaderCompiler.py
