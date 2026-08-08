add_executable(vecgui-button ${SOURCE_FILES} main.cpp)

target_include_directories(vecgui-button PUBLIC "../../")

target_link_libraries(vecgui-button vecgui)
