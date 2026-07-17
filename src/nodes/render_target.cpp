#include "render_target.h"

#include "../servers/render_server.h"
#include "../servers/vector_server.h"

namespace vecgui {

RenderTarget::RenderTarget(const Vec2I size) {
    type = NodeType::RenderTarget;
    size_ = size;

    auto render_server = RenderServer::get_singleton();
    if (render_server->device_) {
        vector_target_ = render_server->device_->create_texture(
            {size_, Pathfinder::TextureFormat::Rgba8Unorm}, "render target texture");
    }
}

void RenderTarget::update(double dt) {
    // Basic RenderTarget doesn't do much in update.
}

void RenderTarget::pre_draw_propagation() {
    if (!visible_ || !vector_target_) {
        return;
    }

    auto vector_server = VectorServer::get_singleton();

    vector_server->set_global_scale(dpi_scale_);
    vector_server->set_canvas_size(vector_target_->get_size());
    vector_server->set_dst_texture(vector_target_);
}

void RenderTarget::post_draw_propagation() {
    auto vector_server = VectorServer::get_singleton();
    vector_server->submit_and_clear();
}

Vec2I RenderTarget::get_size() const {
    return size_;
}

void RenderTarget::set_size(Vec2I size) {
    if (size_ == size) return;
    size_ = size;

    auto render_server = RenderServer::get_singleton();
    if (render_server->device_) {
        vector_target_ = render_server->device_->create_texture(
            {size_, Pathfinder::TextureFormat::Rgba8Unorm}, "render target texture");
    }
}

} // namespace vecgui
