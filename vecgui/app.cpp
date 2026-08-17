#include "app.h"

#include <pathfinder/prelude.h>

#include <cstdint>
#include <memory>

#include "common/context.h"
#include "resources/default_resource.h"
#include "servers/engine.h"
#include "servers/input_server.h"
#include "servers/render_server.h"
#include "servers/text_server.h"
#include "servers/vector_server.h"

namespace vecgui {

#ifndef __ANDROID__
App::App(Vec2I primary_window_size, bool use_vulkan) {
    // Set logger level.
    Logger::set_global_level(Logger::Level::Info);
    Logger::set_module_level("vecgui", Logger::Level::Info);

    // Initialize all servers.
    engine = std::make_unique<Engine>();
    vector_server = std::make_unique<VectorServer>();
    text_server = std::make_unique<TextServer>();
    input_server = std::make_unique<InputServer>();
    render_context = std::make_unique<RenderContext>();
    default_resource = std::make_unique<DefaultResource>();
    translation_server = std::make_unique<TranslationServer>();

    // Fill the context container.
    context.engine = engine.get();
    context.vector_server = vector_server.get();
    context.text_server = text_server.get();
    context.input_server = input_server.get();
    context.render_context = render_context.get();
    context.default_resource = default_resource.get();
    context.translation_server = translation_server.get();

    #ifdef __APPLE__
    auto backend = Pathfinder::BackendType::Metal;
    #else
    auto backend = Pathfinder::BackendType::Opengl;
    if (use_vulkan) {
        backend = Pathfinder::BackendType::Vulkan;
    }
    #endif

    auto window_builder = Pathfinder::WindowBuilder::new_impl(backend, primary_window_size);

    // Create device and queue.
    auto device = window_builder->request_device();
    auto queue = window_builder->create_queue();

    render_context->init(window_builder, device, queue);

    // Create the main window.
    auto primary_window = window_builder->get_window(0);

    vector_server->init(primary_window.lock()->get_physical_size(), device, queue, Pathfinder::RenderMode::Hybrid);

    // Initialize resources (depends on servers).
    default_resource->init(&context, true);

    tree = std::make_unique<SceneTree>(&context, primary_window_size);
}

#else
App::App(ANativeWindow* native_window, void* asset_manager, Vec2I window_size, bool use_vulkan) {
    // Set logger level.
    Logger::set_global_level(Logger::Level::Info);
    Logger::set_module_level("vecgui", Logger::Level::Info);

    // Initialize all servers.
    engine = std::make_unique<Engine>();
    vector_server = std::make_unique<VectorServer>();
    text_server = std::make_unique<TextServer>();
    input_server = std::make_unique<InputServer>();
    render_context = std::make_unique<RenderContext>();
    default_resource = std::make_unique<DefaultResource>();
    translation_server = std::make_unique<TranslationServer>();

    // Fill the context container.
    context.engine = engine.get();
    context.vector_server = vector_server.get();
    context.text_server = text_server.get();
    context.input_server = input_server.get();
    context.render_context = render_context.get();
    context.default_resource = default_resource.get();
    context.translation_server = translation_server.get();

    engine->asset_manager = asset_manager;

    auto backend = Pathfinder::BackendType::Opengl;
    if (use_vulkan) {
        backend = Pathfinder::BackendType::Vulkan;
    }
    auto window_builder = Pathfinder::WindowBuilder::new_impl(native_window, backend, window_size);

    // Create device and queue.
    auto device = window_builder->request_device();
    auto queue = window_builder->create_queue();

    render_context->init(window_builder, device, queue);

    // Create the main window.
    auto primary_window = render_context->get_window_builder()->get_window(0);

    vector_server->init(primary_window.lock()->get_physical_size(),
                        render_context->get_device(),
                        render_context->get_queue(),
                        Pathfinder::RenderMode::Hybrid);

    // Initialize resources (depends on servers).
    default_resource->init(&context, true);

    tree = std::make_unique<SceneTree>(&context, window_size);

    render_context->get_queue()->wait_idle();
}
#endif

App::~App() {
    // Clean up the scene tree.
    tree.reset();

    vector_server->destroy();
    Logger::verbose("Cleaned up VectorServer.", "vecgui");

    render_context->destroy();
    Logger::verbose("Cleaned up RenderContext.", "vecgui");
}

std::shared_ptr<Node> App::get_tree_root() const {
    return tree->get_root();
}

void App::set_window_title(const std::string& title) {
    auto primary_window = render_context->get_window_builder()->get_window(0);
    primary_window.lock()->set_window_title(title);
}

void App::set_fullscreen(bool fullscreen) {
    render_context->get_window_builder()->set_fullscreen(fullscreen);
}

void App::set_custom_scaling_factor(float new_value) {
    render_context->get_window_builder()->set_dpi_scaling_factor(0, new_value);
}

float App::get_scaling_factor() const {
    return render_context->get_window_builder()->get_dpi_scaling_factor(0);
}

void App::main_loop() {
    bool closing_app = false;

    render_context->get_queue()->wait_idle();

    while (!closing_app) {
        render_context->get_window_builder()->poll_events();

        // Engine processing.
        engine->tick();

        // Get frame time.
        const auto dt = engine->get_dt();

        // Update the scene tree.
        tree->process(dt);

        input_server->clear_events();

        closing_app = tree->render();
    }

    render_context->get_window_builder()->stop_and_destroy_swapchains();
}

bool App::single_run() {
    render_context->get_window_builder()->poll_events();

    // Engine processing.
    engine->tick();

    // Get frame time.
    const auto dt = engine->get_dt();

    // Update the scene tree.
    tree->process(dt);

    input_server->clear_events();

    return tree->render();
}

void App::single_run_cleanup() {
    render_context->get_window_builder()->stop_and_destroy_swapchains();
}

} // namespace vecgui
