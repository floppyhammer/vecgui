#pragma once

#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <queue>
#include <vector>

#include "common/context.h"
#include "common/geometry.h"
#include "nodes/file_dialog.h"
#include "nodes/node.h"
#include "nodes/render_target.h"
#include "nodes/timer.h"
#include "nodes/ui/button/button.h"
#include "nodes/ui/button/check_button.h"
#include "nodes/ui/button/menu_button.h"
#include "nodes/ui/button/radio_button.h"
#include "nodes/ui/container/box_container.h"
#include "nodes/ui/container/collapse_container.h"
#include "nodes/ui/container/grid_container.h"
#include "nodes/ui/container/margin_container.h"
#include "nodes/ui/container/scroll_container.h"
#include "nodes/ui/container/split_container.h"
#include "nodes/ui/container/tab_container.h"
#include "nodes/ui/label.h"
#include "nodes/ui/panel.h"
#include "nodes/ui/popup_menu.h"
#include "nodes/ui/progress_bar.h"
#include "nodes/ui/slider.h"
#include "nodes/ui/spin_box.h"
#include "nodes/ui/text_edit.h"
#include "nodes/ui/texture_rect.h"
#include "nodes/ui/tree.h"
#include "resources/default_resource.h"
#include "scene_tree.h"
#include "servers/engine.h"
#include "servers/input_server.h"
#include "servers/render_server.h"
#include "servers/text_server.h"
#include "servers/translation_server.h"
#include "servers/vector_server.h"

class ANativeWindow;

namespace vecgui {

class App {
public:
#ifndef __ANDROID__
    App(Vec2I primary_window_size, bool use_vulkan = true);
#else
    App(ANativeWindow* native_window, void* asset_manager, Vec2I window_size, bool use_vulkan = true);
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

    const GuiContext& get_context() const {
        return context;
    }

private:
    std::unique_ptr<SceneTree> tree;

    // Server instances
    std::unique_ptr<Engine> engine;
    std::unique_ptr<VectorServer> vector_server;
    std::unique_ptr<TextServer> text_server;
    std::unique_ptr<InputServer> input_server;
    std::unique_ptr<RenderContext> render_context;
    std::unique_ptr<DefaultResource> default_resource;
    std::unique_ptr<TranslationServer> translation_server;

    GuiContext context;
};

} // namespace vecgui
