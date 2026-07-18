#include "proxy_window.h"

#include "../common/geometry.h"
#include "../servers/input_server.h"
#include "../servers/render_server.h"
#include "../servers/vector_server.h"

namespace vecgui {

ProxyWindow::ProxyWindow(const Vec2I size, const int window_index) : RenderTarget(size) {
    type = NodeType::Window;
    window_index_ = window_index;

    auto render_context = RenderContext::get_singleton();

    if (window_index_ == 255) {
        window_index_ = render_context->get_window_builder()->create_window(size_, "Window");
    }

    auto window = render_context->get_window_builder()->get_window(window_index_).lock();

    auto input_server = InputServer::get_singleton();
    input_server->initialize_window_callbacks(window_index_);

    auto swap_chain_ = window->get_swap_chain(render_context->get_device());

    blit_ = std::make_shared<Pathfinder::Blit>(
        render_context->get_device(), render_context->get_queue(), swap_chain_->get_surface_format());
}

void ProxyWindow::update(double dt) {
    auto render_context = RenderContext::get_singleton();
    auto window = render_context->get_window_builder()->get_window(window_index_).lock();

    // Closing a window just hides it.
    if (window->should_close() || !visible_) {
        window->hide();
    } else {
        window->show();
    }
}

void ProxyWindow::pre_draw_propagation() {
    if (!visible_) {
        return;
    }

    auto render_context = RenderContext::get_singleton();
    auto window = render_context->get_window_builder()->get_window(window_index_).lock();

    auto vector_server = VectorServer::get_singleton();

    // Sync physical size and DPI from window.
    dpi_scale_ = window->get_dpi_scaling_factor();
    auto physical_size = window->get_physical_size();

    vector_server->set_global_scale(window->get_dpi_scaling_factor());

    if (!vector_target_ || (physical_size != vector_target_->get_size() && !physical_size.is_any_zero())) {
        vector_target_ = render_context->get_device()->create_texture(
            {physical_size, Pathfinder::TextureFormat::Rgba8Unorm}, "dst texture");

        vector_server->set_canvas_size(physical_size);

        std::ostringstream ss;
        ss << "Vector target of the primary window resized to " << physical_size;
        Logger::info(ss.str(), "vecgui");
    }
}

void ProxyWindow::post_draw_propagation() {
    auto render_context = RenderContext::get_singleton();
    auto vector_server = VectorServer::get_singleton();

    auto window = render_context->get_window_builder()->get_window(window_index_).lock();
    auto swap_chain_ = window->get_swap_chain(render_context->get_device());

    // Acquire next swap chain image.
    if (!swap_chain_->acquire_image()) {
        return;
    }

    render_context->get_device()->begin_frame();
    render_context->get_queue()->begin_frame(render_context->get_device()->get_current_frame_index());

    // Submit vector commands.
    vector_server->set_dst_texture(vector_target_);
    vector_server->submit_and_clear();

    auto encoder = render_context->get_device()->create_command_encoder("Window main encoder");
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
}

void ProxyWindow::set_visibility(bool visible) {
    if (visible_ == visible) {
        return;
    }

    visible_ = visible;
}

std::shared_ptr<Pathfinder::Window> ProxyWindow::get_raw_window() const {
    auto render_server = RenderContext::get_singleton();
    auto window = render_server->get_window_builder()->get_window(window_index_).lock();
    return window;
}

} // namespace vecgui
