#pragma once

#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <queue>
#include <vector>

#include "common/geometry.h"
#include "nodes/scene_tree.h"
#include "servers/translation_server.h"

class ANativeWindow;

namespace vecgui {

class App {
public:
#ifndef __ANDROID__
    App(Vec2I primary_window_size, bool dark_mode, bool use_vulkan = true);
#else
    App(ANativeWindow* native_window, void* asset_manager, Vec2I window_size, bool dark_mode, bool use_vulkan = true);
#endif

    ~App();

    void main_loop();

    bool single_run();

    void single_run_cleanup();

    std::shared_ptr<Node> get_tree_root() const;

    void set_window_title(const std::string& title);

    void set_fullscreen(bool fullscreen);

    void set_custom_scaling_factor(float new_value);

    float get_scaling_factor() const;

private:
    std::unique_ptr<SceneTree> tree;

    bool dark_mode_ = false;
};

} // namespace vecgui
