add_executable(vecgui-progress-bar ${SOURCE_FILES} main.cpp)

target_include_directories(vecgui-progress-bar PUBLIC "../../")

target_link_libraries(vecgui-progress-bar vecgui)
