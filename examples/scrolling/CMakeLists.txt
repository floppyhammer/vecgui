add_executable(vecgui-scrolling ${SOURCE_FILES} main.cpp)

target_include_directories(vecgui-scrolling PUBLIC "../../")

target_link_libraries(vecgui-scrolling vecgui)
