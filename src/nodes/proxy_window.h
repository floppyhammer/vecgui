#pragma once

#include "render_target.h"

#if !defined(VECGUI_USE_OFFSCREEN)
namespace Pathfinder {
class Window;
class Blit;
}
#endif

namespace vecgui {

class ProxyWindow : public RenderTarget {
    friend class SceneTree;

public:
    ProxyWindow(Vec2I size, int window_index);

    void update(double dt) override;

    void pre_draw_propagation() override;

    void post_draw_propagation() override;

    void set_visibility(bool visible) override;

#if !defined(VECGUI_USE_OFFSCREEN)
    std::shared_ptr<Pathfinder::Window> get_raw_window() const;
#endif

protected:
    uint8_t window_index_;
};

} // namespace vecgui
