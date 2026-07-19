cmake_minimum_required(VERSION 3.4.1)

project(vecgui_android_demo)

# Integrate GameActivity, refer to
#     https://d.android.com/games/agdk/integrate-game-activity
# for the detailed instructions.
find_package(game-activity REQUIRED CONFIG)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -frtti -Wall \
                     -DVK_USE_PLATFORM_ANDROID_KHR")

add_subdirectory(../../../../../../ vecgui)
include_directories(../../../../../../)

################### GL
if (${ANDROID_PLATFORM_LEVEL} LESS 12)
    message(FATAL_ERROR "OpenGL 2 is not supported before API level 11 \
                      (currently using ${ANDROID_PLATFORM_LEVEL}).")
    return()
elseif (${ANDROID_PLATFORM_LEVEL} LESS 18)
    add_definitions("-DDYNAMIC_ES3")
    set(GL3STUB_SRC gl3stub.c)
    set(OPENGL_LIB GLESv2)
else ()
    set(OPENGL_LIB GLESv3)
endif ()
################### GL

link_directories(lib/${ANDROID_ABI})

add_library(${CMAKE_PROJECT_NAME} SHARED
        AndroidMain.cpp
        GameActivitySources.cpp)

target_link_libraries(${CMAKE_PROJECT_NAME}
        game-activity::game-activity
        log
        android
        vecgui
        EGL # GL
        ${OPENGL_LIB} # GL
)
