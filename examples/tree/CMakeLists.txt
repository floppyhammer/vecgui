add_executable(vecgui-tree ${SOURCE_FILES} main.cpp)

target_include_directories(vecgui-tree PUBLIC "../../")

target_link_libraries(vecgui-tree vecgui)
