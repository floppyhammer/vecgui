#include "proxy_window.h"

#include "../common/geometry.h"
#include "../servers/render_server.h"
#include "../servers/vector_server.h"
#include "../servers/input_server.h"

#if !defined(VECGUI_USE_OFFSCREEN)
#include <pathfinder/prelude.h>
#endif

namespace vecgui {

ProxyWindow::ProxyWindow(const Vec2I size, const int window_index) : RenderTarget(size) {
    type = NodeType::Window;
    window_index_ = window_index;

#if !defined(VECGUI_USE_OFFSCREEN)
    auto render_server = RenderServer::get_singleton();

    if (window_index_ == 255) {
        window_index_ = render_server->window_builder_->create_window(size_, "Window");
    }

    auto window = render_server->window_builder_->get_window(window_index_).lock();

    auto input_server = InputServer::get_singleton();
    input_server->initialize_window_callbacks(window_index_);

    auto swap_chain_ = window->get_swap_chain(render_server->device_);

    blit_ = std::make_shared<Pathfinder::Blit>(
        render_server->device_, render_server->queue_, swap_chain_->get_surface_format());
#endif
}

void ProxyWindow::update(double dt) {
#if !defined(VECGUI_USE_OFFSCREEN)
    auto render_server = RenderServer::get_singleton();
    auto window = render_server->window_builder_->get_window(window_index_).lock();

    // Closing a window just hides it.
    if (window->should_close() || !visible_) {
        window->hide();
    } else {
        window->show();
    }
#endif
}

void ProxyWindow::pre_draw_propagation() {
    if (!visible_) {
        return;
    }

#if !defined(VECGUI_USE_OFFSCREEN)
    auto render_server = RenderServer::get_singleton();
    auto window = render_server->window_builder_->get_window(window_index_).lock();

    // Sync physical size and DPI from window.
    dpi_scale_ = window->get_dpi_scaling_factor();
    auto physical_size = window->get_physical_size();

    if (physical_size != vector_target_->get_size()) {
        if (!physical_size.is_any_zero()) {
            vector_target_ = render_server->device_->create_texture(
                {physical_size, Pathfinder::TextureFormat::Rgba8Unorm}, "dst texture");

            size_ = physical_size;
        }
    }
#endif

    // Call base class to setup VectorServer.
    RenderTarget::pre_draw_propagation();
}

void ProxyWindow::post_draw_propagation() {
#if !defined(VECGUI_USE_OFFSCREEN)
    auto render_server = RenderServer::get_singleton();
    auto vector_server = VectorServer::get_singleton();

    auto window = render_server->window_builder_->get_window(window_index_).lock();
    auto swap_chain_ = window->get_swap_chain(render_server->device_);

    // Acquire next swap chain image.
    if (!swap_chain_->acquire_image()) {
        return;
    }

    render_server->device_->begin_frame();
    render_server->queue_->begin_frame(render_server->device_->get_current_frame_index());

    // Submit vector commands.
    vector_server->submit_and_clear();

    auto encoder = render_server->device_->create_command_encoder("Window main encoder");
    auto surface_texture = swap_chain_->get_surface_texture();

    // Swap chain render pass.
    {
        encoder->begin_render_pass(swap_chain_->get_render_pass(), surface_texture, ColorF(0.2, 0.2, 0.2, 1.0));
        encoder->set_viewport({{0, 0}, window->get_physical_size()});
        blit_->set_texture(vector_target_);
        blit_->draw(encoder);
        encoder->end_render_pass();
    }

    swap_chain_->submit(encoder);
    swap_chain_->present();
#else
    RenderTarget::post_draw_propagation();
#endif
}

void ProxyWindow::set_visibility(bool visible) {
    if (visible_ == visible) {
        return;
    }

    visible_ = visible;
}

#if !defined(VECGUI_USE_OFFSCREEN)
std::shared_ptr<Pathfinder::Window> ProxyWindow::get_raw_window() const {
    auto render_server = RenderServer::get_singleton();
    auto window = render_server->window_builder_->get_window(window_index_).lock();
    return window;
}
#endif

} // namespace vecgui
