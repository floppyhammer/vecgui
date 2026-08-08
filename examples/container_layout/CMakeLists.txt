add_executable(vecgui-container-layout ${SOURCE_FILES} main.cpp)

target_include_directories(vecgui-container-layout PUBLIC "../../")

target_link_libraries(vecgui-container-layout vecgui)
