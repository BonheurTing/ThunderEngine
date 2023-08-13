rmdir /s /q ".\Intermediate\Build"
mkdir ".\Intermediate\Build"
cd ./Intermediate/Build
cmake -A x64 ../..
echo Config finished...
pause
cmake --build .
echo Build finished...
pause
cmake --install . --config Debug
echo Install finished...
pause