#pragma once

#include <pathfinder/prelude.h>

namespace vecgui {

class RenderServer {
public:
    static RenderServer *get_singleton() {
        static RenderServer singleton;
        return &singleton;
    }

    void destroy() {
        queue_.reset();
        device_.reset();
        window_builder_.reset();
    }

    std::shared_ptr<Pathfinder::WindowBuilder> window_builder_;
    std::shared_ptr<Pathfinder::Device> device_;
    std::shared_ptr<Pathfinder::Queue> queue_;
};

} // namespace vecgui
