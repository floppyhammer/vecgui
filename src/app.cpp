#include "app.h"

#include <cstdint>
#include <memory>

// clang-format off
#ifdef PATHFINDER_USE_OPENGL
#include "pathfinder/gpu/gl/window_builder.h"
#endif
#ifdef PATHFINDER_USE_VULKAN
#include "pathfinder/gpu/vk/window_builder.h"
#endif
// clang-format on

#include "resources/default_resource.h"
#include "servers/engine.h"
#include "servers/input_server.h"
#include "servers/render_server.h"
#include "servers/vector_server.h"

namespace vecgui {

#ifndef __ANDROID__
App::App(Vec2I primary_window_size, const bool dark_mode, bool use_vulkan) {
    // Set logger level.
    Logger::set_global_level(Logger::Level::Info);
    Logger::set_module_level("vecgui", Logger::Level::Info);

    dark_mode_ = dark_mode;

    DefaultResource::get_singleton()->init(dark_mode_);

    auto render_server = RenderServer::get_singleton();

    auto window_builder = Pathfinder::WindowBuilder::new_impl(
        use_vulkan ? Pathfinder::BackendType::Vulkan : Pathfinder::BackendType::Opengl, primary_window_size);

    render_server->window_builder_ = window_builder;

    // Create the main window.
    auto primary_window = render_server->window_builder_->get_window(0);

    // Create device and queue.
    render_server->device_ = window_builder->request_device();
    render_server->queue_ = window_builder->create_queue();

    auto vector_server = VectorServer::get_singleton();
    vector_server->init(primary_window.lock()->get_physical_size(),
                        render_server->device_,
                        render_server->queue_,
                        Pathfinder::RenderLevel::D3d9);

    tree = std::make_unique<SceneTree>(primary_window_size);
}

#else
App::App(ANativeWindow* native_window, void* asset_manager, Vec2I window_size, const bool dark_mode, bool use_vulkan) {
    // Set logger level.
    Logger::set_global_level(Logger::Level::Info);
    Logger::set_module_level("vecgui", Logger::Level::Info);

    dark_mode_ = dark_mode;

    Engine::get_singleton()->asset_manager = asset_manager;

    DefaultResource::get_singleton()->init(dark_mode_);

    auto render_server = RenderServer::get_singleton();

    std::shared_ptr<Pathfinder::WindowBuilder> window_builder;

    if (!use_vulkan) {
        window_builder = std::make_shared<Pathfinder::WindowBuilderGl>(native_window, window_size);
    } else {
        InitVulkan();
        window_builder = std::make_shared<Pathfinder::WindowBuilderVk>(native_window, window_size);
    }

    render_server->window_builder_ = window_builder;

    // Create the main window.
    auto primary_window = render_server->window_builder_->get_window(0);

    // Create device and queue.
    render_server->device_ = window_builder->request_device();
    render_server->queue_ = window_builder->create_queue();

    auto vector_server = VectorServer::get_singleton();
    vector_server->init(primary_window.lock()->get_physical_size(),
                        render_server->device_,
                        render_server->queue_,
                        Pathfinder::RenderLevel::D3d9);

    tree = std::make_unique<SceneTree>(window_size);
}
#endif

App::~App() {
    // Clean up the scene tree.
    tree.reset();

    VectorServer::get_singleton()->cleanup();
    Logger::verbose("Cleaned up VectorServer.", "vecgui");

    RenderServer::get_singleton()->destroy();
    Logger::verbose("Cleaned up RenderServer.", "vecgui");
}

std::shared_ptr<Node> App::get_tree_root() const {
    return tree->get_root();
}

void App::set_window_title(const std::string& title) {
    auto render_server = RenderServer::get_singleton();
    auto primary_window = render_server->window_builder_->get_window(0);
    primary_window.lock()->set_window_title(title);
}

void App::set_fullscreen(bool fullscreen) {
    auto render_server = RenderServer::get_singleton();

    render_server->window_builder_->set_fullscreen(fullscreen);
}

void App::set_custom_scaling_factor(float new_value) {
    auto render_server = RenderServer::get_singleton();

    render_server->window_builder_->set_dpi_scaling_factor(0, new_value);
}

float App::get_scaling_factor() const {
    auto render_server = RenderServer::get_singleton();

    return render_server->window_builder_->get_dpi_scaling_factor(0);
}

void App::main_loop() {
    bool closing_app = false;

    while (!closing_app) {
        RenderServer::get_singleton()->window_builder_->poll_events();

        // Engine processing.
        Engine::get_singleton()->tick();

        // Get frame time.
        auto dt = Engine::get_singleton()->get_dt();

        // Update the scene tree.
        tree->process(dt);

        InputServer::get_singleton()->clear_events();

        closing_app = tree->render();
    }

    RenderServer::get_singleton()->window_builder_->stop_and_destroy_swapchains();
}

bool App::single_run() {
    RenderServer::get_singleton()->window_builder_->poll_events();

    // Engine processing.
    Engine::get_singleton()->tick();

    // Get frame time.
    auto dt = Engine::get_singleton()->get_dt();

    // Update the scene tree.
    tree->process(dt);

    InputServer::get_singleton()->clear_events();

    return tree->render();
}

void App::single_run_cleanup() {
    RenderServer::get_singleton()->window_builder_->stop_and_destroy_swapchains();
}

} // namespace vecgui
