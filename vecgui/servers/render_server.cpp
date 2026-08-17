#include "render_server.h"

namespace vecgui {

struct RenderContextImpl {
#ifdef VECGUI_USE_WINDOW
    std::shared_ptr<Pathfinder::WindowBuilder> window_builder_;
#endif
    std::shared_ptr<Pathfinder::Device> device_;
    std::shared_ptr<Pathfinder::Queue> queue_;
};

RenderContext::RenderContext() = default;
RenderContext::~RenderContext() = default;

void RenderContext::init(std::shared_ptr<Pathfinder::WindowBuilder> window_builder,
                         std::shared_ptr<Pathfinder::Device> device,
                         std::shared_ptr<Pathfinder::Queue> queue) {
    if (impl_) {
        return;
    }

    impl_ = std::make_unique<RenderContextImpl>();
#ifdef VECGUI_USE_WINDOW
    impl_->window_builder_ = window_builder;
#endif
    impl_->device_ = device;
    impl_->queue_ = queue;
}

void RenderContext::destroy() {
    impl_.reset();
}

std::shared_ptr<Pathfinder::WindowBuilder> RenderContext::get_window_builder() const {
#ifdef VECGUI_USE_WINDOW
    return impl_->window_builder_;
#else
    return nullptr;
#endif
}

std::shared_ptr<Pathfinder::Device> RenderContext::get_device() const {
    return impl_->device_;
}

std::shared_ptr<Pathfinder::Queue> RenderContext::get_queue() const {
    return impl_->queue_;
}

} // namespace vecgui
