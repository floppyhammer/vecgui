cmake_minimum_required(VERSION 3.16)

project(vecgui
        VERSION 0.1
        LANGUAGES C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Identify Linux.
if (UNIX AND NOT APPLE AND NOT ANDROID)
    set(LINUX ON)
endif ()

option(VECGUI_VULKAN "Use Vulkan instead of OpenGL" ON)
option(VECGUI_FRIBIDI "Use fribidi instead of icu" ON)
option(VECGUI_BUILD_EXAMPLES "Build native examples" OFF)

file(GLOB_RECURSE VECGUI_SRC_FILES CONFIGURE_DEPENDS
        src/*.cpp
        third_party/tinyfiledialogs/tinyfiledialogs.c
)

if (APPLE)
    enable_language(OBJC OBJCXX)

    file(GLOB_RECURSE VECGUI_SRC_FILES_APPLE CONFIGURE_DEPENDS
            src/*.mm
    )

    file(GLOB VECGUI_SRC_FILES ${VECGUI_SRC_FILES} ${VECGUI_SRC_FILES_APPLE})

    set(VECGUI_FRIBIDI ON)
endif ()

add_library(vecgui STATIC ${VECGUI_SRC_FILES})

file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})

# Has to be 20 or above.
target_compile_features(vecgui PUBLIC cxx_std_20)

if (MSVC)
    target_compile_options(vecgui PUBLIC "/utf-8")
endif ()

# Include external and third-party headers.
target_include_directories(vecgui PUBLIC "third_party")

# Do this before add_subdirectory
if (VECGUI_VULKAN)
    set(PATHFINDER_BACKEND_VULKAN ON)
else ()
    set(PATHFINDER_BACKEND_VULKAN OFF)
endif ()
set(PATHFINDER_USE_D3D11 OFF)
target_include_directories(vecgui PUBLIC "third_party/pathfinder-cpp")
add_subdirectory("third_party/pathfinder-cpp")

target_include_directories(vecgui PUBLIC "third_party/harfbuzz-11.3.2/src")
add_subdirectory("third_party/harfbuzz-11.3.2")

target_include_directories(vecgui PUBLIC "third_party/GSL-4.0.0/include")
add_subdirectory("third_party/GSL-4.0.0")

target_include_directories(vecgui PUBLIC "third_party/rapidcsv-8.84/src")
add_subdirectory("third_party/rapidcsv-8.84")

target_include_directories(vecgui PUBLIC "third_party/stduuid-1.2.3")

set(ZLIB_BUILD_EXAMPLES OFF)
add_subdirectory("third_party/zlib")

target_link_libraries(vecgui PUBLIC pathfinder harfbuzz zlibstatic)

if (LINUX)
    target_link_libraries(vecgui PUBLIC tbb)
endif ()

if (NOT VECGUI_FRIBIDI)
    if (WIN32)
        # Fix "too many characters in constant" error when using CMake with MSVC.
        string(APPEND CMAKE_CXX_FLAGS " /utf-8")

        target_link_libraries(vecgui PUBLIC icu)
    elseif (LINUX)
        # icu4c.
        find_package(ICU 70.0 COMPONENTS uc i18n REQUIRED)

        target_link_libraries(vecgui PUBLIC ICU::uc ICU::i18n)
    endif ()
endif ()

if (VECGUI_FRIBIDI)
    # Alternative to icu4c. If the target platform comes with ICU, we just use it.
    # Else, we build fribidi. It's too cumbersome to build ICU ourselves.
    add_subdirectory("third_party/fribidi-1.0.16")
    target_link_libraries(vecgui PUBLIC fribidi)

    target_compile_definitions(vecgui PUBLIC VECGUI_USE_FRIBIDI)
endif ()

# Copy the assets to the binary directory.
file(COPY "assets" DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# Build examples.
if (VECGUI_BUILD_EXAMPLES)
    add_subdirectory(examples/file_selection)
    add_subdirectory(examples/container_layout)
    add_subdirectory(examples/texture_rect)
    add_subdirectory(examples/scrolling)
    add_subdirectory(examples/tab)
    add_subdirectory(examples/progress_bar)
    add_subdirectory(examples/i18n)
    add_subdirectory(examples/rich_text)
    add_subdirectory(examples/label_alignment)
    add_subdirectory(examples/multiple_window)
    add_subdirectory(examples/button)
    add_subdirectory(examples/tree)
    add_subdirectory(examples/popup_menu)
    add_subdirectory(examples/collapse_containers)
    add_subdirectory(examples/split_container)
    add_subdirectory(examples/slider)
    add_subdirectory(examples/anchor_flag)
endif ()
