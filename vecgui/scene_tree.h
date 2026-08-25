#pragma once

#include <thread>

#include "common/context.h"
#include "nodes/node.h"
#include "nodes/render_target.h"
#include "nodes/ui/button/button.h"

namespace Pathfinder {
class Window;
}

namespace vecgui {

class ProxyWindow;

void transform_system(Node* base);

void propagate_draw(Node* base);

/// Run calc_minimum_size() depth-first.
void calc_minimum_size(Node* base);

void layout_system(Node* base);

/// Processing order: Input -> Update -> Draw.
class SceneTree {
    friend class App;

public:
    SceneTree(GuiContext* context, Vec2I primary_window_size);

    explicit SceneTree(GuiContext* context);

    void process(double dt);

    bool render() const;

    std::shared_ptr<Node> get_root() const;

    void notify_primary_window_size_changed(Vec2I new_size) const;

    void quit();

    std::shared_ptr<Pathfinder::Window> get_primary_window() const;

    Vec2I get_view_size() const;

    float get_dpi_scale() const;

    GuiContext* get_context() const {
        return context;
    }

private:
    /// Primary render target (can be a window or a generic target)
    std::shared_ptr<RenderTarget> root;

    GuiContext* context = nullptr;

    bool quited = false;
};

} // namespace vecgui
