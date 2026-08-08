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
option(VECGUI_WINDOW "Build with GLFW dependency and window management" ON)

file(GLOB_RECURSE VECGUI_SRC_FILES CONFIGURE_DEPENDS
        vecgui/*.cpp
        third_party/tinyfiledialogs/tinyfiledialogs.c
)

if (NOT VECGUI_WINDOW)
    list(REMOVE_ITEM VECGUI_SRC_FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/vecgui/app.cpp"
    )
    list(REMOVE_ITEM VECGUI_SRC_FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/vecgui/nodes/proxy_window.cpp"
    )
endif ()

# All headers
file(GLOB_RECURSE VECGUI_HDR_FILES CONFIGURE_DEPENDS
        vecgui/*.h
)

# Collect pathfinder headers that are needed by public API.
file(GLOB_RECURSE PATHFINDER_HDR_FILES CONFIGURE_DEPENDS
        third_party/pathfinder-cpp/pathfinder/*.h
)

if (APPLE)
    enable_language(OBJC OBJCXX)

    file(GLOB_RECURSE VECGUI_SRC_FILES_APPLE CONFIGURE_DEPENDS
            vecgui/*.mm
    )

    file(GLOB VECGUI_SRC_FILES ${VECGUI_SRC_FILES} ${VECGUI_SRC_FILES_APPLE} ${VECGUI_HDR_FILES} ${PATHFINDER_HDR_FILES})

    set(VECGUI_FRIBIDI ON)
endif ()

# Change to SHARED if you need to build an APPLE framework.
add_library(vecgui STATIC ${VECGUI_SRC_FILES})

# Copy the assets to the runtime output directory.
add_custom_command(TARGET vecgui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets
)

if (VECGUI_WINDOW)
    target_compile_definitions(vecgui PUBLIC VECGUI_USE_WINDOW)
endif ()

if (APPLE)
    # Preserve directory structure for vecgui headers.
    foreach (HDR ${VECGUI_HDR_FILES})
        get_filename_component(HDR_DIR "${HDR}" DIRECTORY)
        file(RELATIVE_PATH REL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vecgui" "${HDR_DIR}")

        if (REL_DIR STREQUAL "")
            set_source_files_properties("${HDR}" PROPERTIES MACOSX_PACKAGE_LOCATION "Headers/vecgui")
        else ()
            set_source_files_properties("${HDR}" PROPERTIES MACOSX_PACKAGE_LOCATION "Headers/vecgui/${REL_DIR}")
        endif ()
    endforeach ()

    # Preserve directory structure for pathfinder headers.
    foreach (HDR ${PATHFINDER_HDR_FILES})
        get_filename_component(HDR_DIR "${HDR}" DIRECTORY)
        file(RELATIVE_PATH REL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/pathfinder-cpp" "${HDR_DIR}")
        set_source_files_properties("${HDR}" PROPERTIES MACOSX_PACKAGE_LOCATION "Headers/${REL_DIR}")
    endforeach ()

    set_target_properties(vecgui PROPERTIES
            FRAMEWORK TRUE
            MACOSX_FRAMEWORK_IDENTIFIER com.vecgui.framework
            MACOSX_FRAMEWORK_BUNDLE_VERSION "0.1.0"
            MACOSX_FRAMEWORK_SHORT_VERSION_STRING "0.1"
            RESOURCE "assets"
            XCODE_ATTRIBUTE_DEFINES_MODULE "YES"
            XCODE_ATTRIBUTE_SKIP_INSTALL "NO"
    )
endif ()

# Has to be 20 or above.
target_compile_features(vecgui PUBLIC cxx_std_20)

if (MSVC)
    target_compile_options(vecgui PUBLIC "/utf-8")
endif ()

# Debug information settings
if (APPLE)
    # 1. Use global compile options so that sub-libraries (pathfinder, etc.) also get debug symbols
    add_compile_options("$<$<CONFIG:Debug>:-g>")
    add_compile_options("$<$<CONFIG:Debug>:-gdwarf-4>")

    # 2. Configure target properties for vecgui
    # We remove -fdebug-prefix-map to allow Xcode to find local source files by absolute path.

    set_target_properties(vecgui PROPERTIES
            # Preserve debug symbols in the framework for Debug config
            XCODE_ATTRIBUTE_STRIP_DEBUG_SYMBOLS_DURING_COPY "$<$<CONFIG:Debug>:NO>$<$<CONFIG:Release>:YES>"
            XCODE_ATTRIBUTE_COPY_PHASE_STRIP "$<$<CONFIG:Debug>:NO>$<$<CONFIG:Release>:YES>"
            XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT "$<$<CONFIG:Debug>:NO>$<$<CONFIG:Release>:YES>"

            # Use 'dwarf' for Debug to keep symbols inside the binary (easier for local dev)
            # Use 'dwarf-with-dsym' for Release to generate external symbol files
            XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug>:dwarf>$<$<CONFIG:Release>:dwarf-with-dsym>"

            XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "$<$<CONFIG:Debug>:YES>$<$<CONFIG:Release>:NO>"
    )
endif ()

# Include external and third-party headers.
target_include_directories(vecgui PUBLIC "." "third_party")

# Do this before add_subdirectory
if (VECGUI_VULKAN)
    set(PATHFINDER_BACKEND_VULKAN ON)
else ()
    set(PATHFINDER_BACKEND_VULKAN OFF)
endif ()
set(PATHFINDER_USE_D3D11 OFF)
if (VECGUI_WINDOW)
    set(PATHFINDER_WINDOW ON)
else ()
    set(PATHFINDER_WINDOW OFF)
endif ()
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

if (APPLE)
    target_link_libraries(vecgui PUBLIC pathfinder harfbuzz zlibstatic "-framework Metal" "-framework QuartzCore" "-framework CoreText"
            "-framework CoreGraphics" "-framework MetalKit" "-framework Foundation")
    target_link_options(vecgui PUBLIC "-Wl,-force_load,$<TARGET_FILE:pathfinder>")

    if (VECGUI_WINDOW)
        target_link_libraries(vecgui PUBLIC "-framework Cocoa")
    endif ()
else ()
    target_link_libraries(vecgui PUBLIC "-Wl,--whole-archive" pathfinder "-Wl,--no-whole-archive" harfbuzz zlibstatic)
endif ()

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
