mkdir -p build && cd build

cmake .. -DVECGUI_OFFSCREEN=ON

cmake --build . --config Release