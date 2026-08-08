add_executable(vecgui-anchor-flag ${SOURCE_FILES} main.cpp)

target_include_directories(vecgui-anchor-flag PUBLIC "../../")

target_link_libraries(vecgui-anchor-flag vecgui)
