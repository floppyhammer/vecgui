mkdir -p build-debug-ios-sim && cd build-debug-ios-sim

cmake .. -DVECGUI_WINDOW=OFF -DCMAKE_BUILD_TYPE=DEBUG -DCMAKE_TOOLCHAIN_FILE=ios.toolchain.cmake -DPLATFORM=SIMULATORARM64 -DENABLE_VISIBILITY=ON

cmake --build . --config DEBUG
