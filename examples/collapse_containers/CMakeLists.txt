add_executable(vecgui-collapse-containers ${SOURCE_FILES} main.cpp)

target_include_directories(vecgui-collapse-containers PUBLIC "../../")

target_link_libraries(vecgui-collapse-containers vecgui)
