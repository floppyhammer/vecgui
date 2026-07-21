mkdir -p build-release-ios-sim && cd build-release-ios-sim

cmake .. -DVECGUI_WINDOW=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=ios.toolchain.cmake -DPLATFORM=SIMULATORARM64 -DENABLE_VISIBILITY=ON

cmake --build . --config Release
