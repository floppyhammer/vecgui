add_executable(vecgui-popup-menu ${SOURCE_FILES} main.cpp)

target_include_directories(vecgui-popup-menu PUBLIC "../../")

target_link_libraries(vecgui-popup-menu vecgui)
