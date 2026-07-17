#include "render_target.h"

#include "../servers/render_server.h"
#include "../servers/vector_server.h"

namespace vecgui {

RenderTarget::RenderTarget(const Vec2I size) {
    type = NodeType::RenderTarget;
    size_ = size;

    auto render_server = RenderServer::get_singleton();
    if (render_server->device_) {
        vector_target_ = render_server->device_->create_texture({size_, Pathfinder::TextureFormat::Rgba8Unorm},
                                                                "render target texture");
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
    auto render_server = RenderServer::get_singleton();

    // Submit vector commands.
    vector_server->submit_and_clear();

    if (!blit_target_) {
        return;
    }

    auto encoder = render_server->device_->create_command_encoder("blit encoder");

    // Swap chain render pass.
    {
        encoder->begin_render_pass(blit_render_pass_, blit_target_, ColorF());
        encoder->set_viewport({{0, 0}, blit_target_->get_size()});
        blit_->set_texture(vector_target_);
        blit_->draw(encoder);
        encoder->end_render_pass();
    }

    render_server->queue_->submit(encoder, nullptr);
}

Vec2I RenderTarget::get_size() const {
    return size_;
}

void RenderTarget::set_size(Vec2I size) {
    if (size_ == size) return;
    size_ = size;

    auto render_server = RenderServer::get_singleton();
    if (render_server->device_) {
        vector_target_ = render_server->device_->create_texture({size_, Pathfinder::TextureFormat::Rgba8Unorm},
                                                                "render target texture");
    }
}

void RenderTarget::set_blit_texture(std::shared_ptr<Pathfinder::Texture> texture) {
    blit_target_ = texture;

    if (blit_target_) {
        if (!blit_) {
            auto render_server = RenderServer::get_singleton();
            blit_ = std::make_shared<Pathfinder::Blit>(
                render_server->device_, render_server->queue_, texture->get_format());

            blit_render_pass_ = render_server->device_->create_render_pass(
                texture->get_format(), Pathfinder::AttachmentLoadOp::Load, "blit render pass load");
        }
    }
}

} // namespace vecgui
