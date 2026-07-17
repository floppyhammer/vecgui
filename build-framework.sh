mkdir -p build-relase-macos && cd build-relase-macos

cmake .. -DVECGUI_OFFSCREEN=ON -DCMAKE_BUILD_TYPE=Release

cmake --build . --config Release
