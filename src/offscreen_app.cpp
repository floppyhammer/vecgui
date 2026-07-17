#include "offscreen_app.h"

#include "nodes/proxy_window.h"
#include "resources/default_resource.h"
#include "servers/engine.h"
#include "servers/input_server.h"
#include "servers/render_server.h"
#include "servers/vector_server.h"

namespace vecgui {

OffscreenApp::OffscreenApp(std::shared_ptr<Pathfinder::Device> device,
                           std::shared_ptr<Pathfinder::Queue> queue,
                           Vec2I size,
                           bool dark_mode) {
    dark_mode_ = dark_mode;

    // Initialize Servers with provided hardware interface.
    auto render_server = RenderServer::get_singleton();
    render_server->device_ = device;
    render_server->queue_ = queue;

    DefaultResource::get_singleton()->init(dark_mode_);

    auto vector_server = VectorServer::get_singleton();
    vector_server->init(size, device, queue, Pathfinder::RenderMode::Hybrid);

    // Create a SceneTree with a headless ProxyWindow as root.
    tree = std::make_unique<SceneTree>();

    // Ensure root has correct initial size.
    std::dynamic_pointer_cast<ProxyWindow>(tree->get_root())->when_parent_size_changed(size.to_f32());
}

OffscreenApp::~OffscreenApp() {
    tree.reset();
    VectorServer::get_singleton()->cleanup();
    RenderServer::get_singleton()->destroy();
}

void OffscreenApp::update(double dt) {
    // Engine processing.
    Engine::get_singleton()->tick();

    // Update the scene tree.
    tree->process(dt);

    InputServer::get_singleton()->clear_events();
}

void OffscreenApp::render(std::shared_ptr<Pathfinder::Texture> target_texture) {
    auto vector_server = VectorServer::get_singleton();
    auto root_window = std::dynamic_pointer_cast<ProxyWindow>(tree->get_root());

    // 1. Prepare target
    root_window->set_vector_target(target_texture);

    // 2. Setup VectorServer via ProxyWindow's logic (sets scale and target)
    root_window->pre_draw_propagation();

    // 3. Collect and draw all nodes.
    propagate_draw(root_window.get());

    // 4. Submit and clear for this frame.
    vector_server->submit_and_clear();
}

std::shared_ptr<Node> OffscreenApp::get_tree_root() const {
    return tree->get_root();
}

} // namespace vecgui
