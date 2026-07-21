mkdir -p build-release-macos && cd build-release-macos

cmake .. -DVECGUI_WINDOW=OFF -DCMAKE_BUILD_TYPE=Release

cmake --build . --config Release
