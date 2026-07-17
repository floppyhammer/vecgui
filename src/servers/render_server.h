#pragma once

#include <pathfinder/prelude.h>

namespace vecgui {

struct RenderContextImpl;

class RenderContext {
public:
    static RenderContext *get_singleton();

    void init(std::shared_ptr<Pathfinder::WindowBuilder> window_builder,
              std::shared_ptr<Pathfinder::Device> device,
              std::shared_ptr<Pathfinder::Queue> queue);

    void destroy();

    std::shared_ptr<Pathfinder::WindowBuilder> get_window_builder() const;

    std::shared_ptr<Pathfinder::Device> get_device() const;
    std::shared_ptr<Pathfinder::Queue> get_queue() const;

    std::unique_ptr<RenderContextImpl> impl_;
};

} // namespace vecgui
