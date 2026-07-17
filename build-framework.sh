mkdir -p build && cd build

cmake .. -DVECGUI_OFFSCREEN=ON -DCMAKE_BUILD_TYPE=Release

cmake --build . --config Release