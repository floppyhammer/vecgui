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
    // Initialize Servers.
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

    render_context->init(nullptr, device, queue);
    vector_server->init(size, device, queue, Pathfinder::RenderMode::Hybrid);

    // Initialize resources.
    default_resource->init(&context, true);

    // Create a SceneTree with a generic RenderTarget as root.
    tree = std::make_unique<SceneTree>(&context);

    // Ensure root has correct initial size.
    auto root = std::dynamic_pointer_cast<RenderTarget>(tree->get_root());
    root->set_size(size);
    root->when_parent_size_changed(size.to_f32());
}

OffscreenApp::~OffscreenApp() {
    tree.reset();
    vector_server->destroy();
    render_context->destroy();
}

void OffscreenApp::update(double dt) {
    // Engine processing.
    engine->tick();

    // Update the scene tree.
    tree->process(dt);

    input_server->clear_events();
}

void OffscreenApp::render(std::shared_ptr<Pathfinder::Texture> target_texture) {
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
    text_server->register_fallback_font(script, font);
}

} // namespace vecgui
