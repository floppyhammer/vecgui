add_executable(vecgui-label-alignment ${SOURCE_FILES} main.cpp)

target_include_directories(vecgui-label-alignment PUBLIC "../../")

target_link_libraries(vecgui-label-alignment vecgui)
