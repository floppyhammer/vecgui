mkdir -p build-relase-ios && cd build-relase-ios

cmake .. -DVECGUI_WINDOW=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=ios.toolchain.cmake -DPLATFORM=OS64 -DENABLE_VISIBILITY=ON

cmake --build . --config Release
