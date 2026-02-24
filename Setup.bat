@echo off

if "%1"=="-shaderlang" (
    echo -- Running ShaderLang compiler generation only
    call .\Tools\ShaderLang\GenerateShaderCompiler.bat
) else (
    echo -- Running full project setup
    python .\Tools\Build\Setup.py
)

pause
