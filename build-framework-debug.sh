mkdir -p build-debug-macos && cd build-debug-macos

cmake .. -DVECGUI_WINDOW=OFF -DCMAKE_BUILD_TYPE=Debug

cmake --build . --config Debug
