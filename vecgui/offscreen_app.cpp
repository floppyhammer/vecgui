#include "offscreen_app.h"

#include "nodes/render_target.h"
#include "resources/default_resource.h"
#include "servers/engine.h"
#include "servers/input_server.h"
#include "servers/render_server.h"
#include "servers/text_server.h"
#include "servers/vector_server.h"

namespace vecgui {

OffscreenApp::OffscreenApp(std::shared_ptr<Pathfinder::Device> device,
                           std::shared_ptr<Pathfinder::Queue> queue,
                           Vec2I size) {
    // Initialize Servers with provided hardware interface.
    auto render_context = RenderContext::get_singleton();
    render_context->init(nullptr, device, queue);

    DefaultResource::get_singleton()->init(true);

    auto vector_server = VectorServer::get_singleton();
    vector_server->init(size, device, queue, Pathfinder::RenderMode::Hybrid);

    // Create a SceneTree with a generic RenderTarget as root.
    tree = std::make_unique<SceneTree>();

    // Ensure root has correct initial size.
    auto root = std::dynamic_pointer_cast<RenderTarget>(tree->get_root());
    root->set_size(size);
    root->when_parent_size_changed(size.to_f32());
}

OffscreenApp::~OffscreenApp() {
    tree.reset();
    VectorServer::get_singleton()->cleanup();
    RenderContext::get_singleton()->destroy();
}

void OffscreenApp::update(double dt) {
    // Engine processing.
    Engine::get_singleton()->tick();

    // Update the scene tree.
    tree->process(dt);

    InputServer::get_singleton()->clear_events();
}

void OffscreenApp::render(std::shared_ptr<Pathfinder::Texture> target_texture) {
    auto render_context = RenderContext::get_singleton();
    render_context->get_device()->begin_frame();
    render_context->get_queue()->begin_frame(render_context->get_device()->get_current_frame_index());

    auto root_target = std::dynamic_pointer_cast<RenderTarget>(tree->get_root());

    root_target->set_blit_texture(target_texture);

    // 2. Setup VectorServer via RenderTarget's logic (sets scale and target)
    root_target->pre_draw_propagation();

    // 3. Collect and draw all nodes.
    propagate_draw(root_target.get());

    // 4. Submit and clear for this frame.
    root_target->post_draw_propagation();
}

void OffscreenApp::set_render_target_size(Vec2I size) {
    auto root = std::dynamic_pointer_cast<RenderTarget>(tree->get_root());
    root->set_size(size);
    root->when_parent_size_changed(size.to_f32());
}

std::shared_ptr<Node> OffscreenApp::get_tree_root() const {
    return tree->get_root();
}

void OffscreenApp::register_fallback_font(Script script, const std::shared_ptr<Font> &font) {
    auto text_server = TextServer::get_singleton();
    text_server->register_fallback_font(script, font);
}

} // namespace vecgui
