#pragma once

#include <thread>

#include "file_dialog.h"
#include "node.h"
#include "render_target.h"
#include "timer.h"
#include "ui/button/button.h"
#include "ui/button/check_button.h"
#include "ui/button/menu_button.h"
#include "ui/button/radio_button.h"
#include "ui/container/box_container.h"
#include "ui/container/collapse_container.h"
#include "ui/container/grid_container.h"
#include "ui/container/margin_container.h"
#include "ui/container/scroll_container.h"
#include "ui/container/split_container.h"
#include "ui/container/tab_container.h"
#include "ui/label.h"
#include "ui/subtitle/subtitle.h"
#include "ui/panel.h"
#include "ui/popup_menu.h"
#include "ui/progress_bar.h"
#include "ui/slider.h"
#include "ui/spin_box.h"
#include "ui/text_edit.h"
#include "ui/texture_rect.h"
#include "ui/tree.h"

namespace Pathfinder {
class Window;
}

namespace vecgui {

class ProxyWindow;

void transform_system(Node* root);

void propagate_draw(Node* node);

/// Run calc_minimum_size() depth-first.
void calc_minimum_size(Node* root);

void layout_system(Node* root);

/// Processing order: Input -> Update -> Draw.
class SceneTree {
    friend class App;

public:
    explicit SceneTree(Vec2I primary_window_size);

    SceneTree();

    void process(double dt);

    bool render() const;

    std::shared_ptr<Node> get_root() const;

    void notify_primary_window_size_changed(Vec2I new_size) const;

    void quit();

    std::shared_ptr<Pathfinder::Window> get_primary_window() const;

    Vec2I get_view_size() const;

    float get_dpi_scale() const;

private:
    /// Primary render target (can be a window or a generic target)
    std::shared_ptr<RenderTarget> root;

    bool quited = false;

    // todo
    std::thread render_thread;
};

} // namespace vecgui
