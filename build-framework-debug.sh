mkdir -p build && cd build

cmake .. -DVECGUI_OFFSCREEN=ON -DCMAKE_BUILD_TYPE=Debug

cmake --build . --config Debug