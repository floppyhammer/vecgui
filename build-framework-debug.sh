mkdir -p build && cd build

cmake .. -DVECGUI_WINDOW=OFF -DCMAKE_BUILD_TYPE=Debug

cmake --build . --config Debug
