@echo off
rmdir /s /q ".\Intermediate\Build"
mkdir ".\Intermediate\Build"
cd ./Intermediate/Build
cmake -A x64 ../..
echo Config finished...
pause
rem cmake --build .
rem echo Build finished...
rem pause
rem cmake --install . --config Debug
rem echo Install finished...
rem pause