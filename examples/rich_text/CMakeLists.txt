add_executable(vecgui-rich-text ${SOURCE_FILES} main.cpp)

target_include_directories(vecgui-rich-text PUBLIC "../../")

target_link_libraries(vecgui-rich-text vecgui)
