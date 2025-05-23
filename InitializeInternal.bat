@echo off
echo -- Clear and Create .\Intermediate\Build
rmdir /s /q ".\Intermediate\Build"
mkdir ".\Intermediate\Build"

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