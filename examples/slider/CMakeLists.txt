add_executable(vecgui-slider ${SOURCE_FILES} main.cpp)

target_include_directories(vecgui-slider PUBLIC "../../")

target_link_libraries(vecgui-slider vecgui)
