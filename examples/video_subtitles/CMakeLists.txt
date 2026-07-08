add_executable(vecgui-video-subtitles ${SOURCE_FILES} main.cpp)

target_include_directories(vecgui-video-subtitles PUBLIC "../../src")

target_link_libraries(vecgui-video-subtitles vecgui)
