cd ./Intermediate/Build
cmake --clean
cmake ../..
echo Config finished...
pause
cmake --build .
echo Build finished...
pause
cmake --install . --config Debug
echo Install finished...
pause