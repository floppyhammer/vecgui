add_executable(vecgui-split-container ${SOURCE_FILES} main.cpp)

target_include_directories(vecgui-split-container PUBLIC "../../")

target_link_libraries(vecgui-split-container vecgui)
