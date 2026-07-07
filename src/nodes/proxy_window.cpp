#include "proxy_window.h"

#include "../common/geometry.h"
#include "../servers/render_server.h"
#include "../servers/vector_server.h"

namespace vecgui {

ProxyWindow::ProxyWindow(const Vec2I size, const int window_index) {
    type = NodeType::Window;

    size_ = size;

    auto render_server = RenderServer::get_singleton();

    if (window_index > -1) {
        window_index_ = window_index;
    } else {
        window_index_ = render_server->window_builder_->create_window(size_, "Window");
    }

    auto window = render_server->window_builder_->get_window(window_index_).lock();

    auto input_server = InputServer::get_singleton();
    input_server->initialize_window_callbacks(window_index_);

    auto swap_chain_ = window->get_swap_chain(render_server->device_);

    blit_ = std::make_shared<Blit>(render_server->device_, render_server->queue_, swap_chain_->get_surface_format());

    vector_target_ =
        render_server->device_->create_texture({size_, Pathfinder::TextureFormat::Rgba8Unorm}, "dst texture");
}

Vec2I ProxyWindow::get_size() const {
    return size_;
}

void ProxyWindow::update(double dt) {
    auto render_server = RenderServer::get_singleton();
    auto window = render_server->window_builder_->get_window(window_index_).lock();

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

    auto render_server = RenderServer::get_singleton();
    auto vector_server = VectorServer::get_singleton();

    auto window = render_server->window_builder_->get_window(window_index_).lock();

    // Set DPI.
    vector_server->set_global_scale(window->get_dpi_scaling_factor());

    auto physical_size = window->get_physical_size();

    if (physical_size != vector_target_->get_size()) {
        if (!physical_size.is_any_zero()) {
            // Texture & canvas should use the physical size.
            auto physical_size = window->get_physical_size();

            vector_target_ = render_server->device_->create_texture(
                {physical_size, Pathfinder::TextureFormat::Rgba8Unorm}, "dst texture");

            vector_server->set_canvas_size(physical_size);

            std::ostringstream ss;
            ss << "Vector target of the primary window resized to " << physical_size;
            Logger::info(ss.str(), "vecgui");
        }
    }

    vector_server->set_dst_texture(vector_target_);

    // temp_draw_data.previous_scene = vector_server->get_canvas()->take_scene();
}

void ProxyWindow::post_draw_propagation() {
    auto render_server = RenderServer::get_singleton();
    auto vector_server = VectorServer::get_singleton();

    auto window = render_server->window_builder_->get_window(window_index_).lock();
    auto swap_chain_ = window->get_swap_chain(render_server->device_);

    // Acquire next swap chain image.
    if (!swap_chain_->acquire_image()) {
        return;
    }

    vector_server->submit_and_clear();

    // vector_server->get_canvas()->set_scene(temp_draw_data.previous_scene);

    auto encoder = render_server->device_->create_command_encoder("Window main encoder");

    auto surface_texture = swap_chain_->get_surface_texture();

    // Swap chain render pass.
    {
        encoder->begin_render_pass(swap_chain_->get_render_pass(), surface_texture, ColorF(0.2, 0.2, 0.2, 1.0));

        encoder->set_viewport({{0, 0}, window->get_physical_size()});

        blit_->set_texture(vector_target_);

        // Draw canvas to screen.
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
    auto render_server = RenderServer::get_singleton();

    auto window = render_server->window_builder_->get_window(window_index_).lock();

    return window;
}

} // namespace vecgui
