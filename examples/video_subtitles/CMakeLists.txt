add_executable(vecgui-video-subtitles ${SOURCE_FILES} main.cpp
        subtitle_style_config.cpp
        subtitle_style_config.h)

target_include_directories(vecgui-video-subtitles PUBLIC "../../src")

target_link_libraries(vecgui-video-subtitles vecgui)
