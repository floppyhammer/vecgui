#pragma once

#include <memory>

#include "common/context.h"
#include "common/geometry.h"
#include "nodes/node.h"
#include "resources/default_resource.h"
#include "scene_tree.h"
#include "servers/engine.h"
#include "servers/input_server.h"
#include "servers/render_server.h"
#include "servers/text_server.h"
#include "servers/translation_server.h"
#include "servers/vector_server.h"

namespace Pathfinder {
class Device;
class Queue;
class Texture;
} // namespace Pathfinder

namespace vecgui {

class OffscreenApp {
public:
    OffscreenApp(std::shared_ptr<Pathfinder::Device> device, std::shared_ptr<Pathfinder::Queue> queue, Vec2I size);

    ~OffscreenApp();

    /**
     * Update the scene tree.
     * @param dt Delta time in seconds.
     */
    void update(double dt);

    /**
     * Render the scene tree to the given texture.
     * @param target_texture The texture to render into.
     */
    void render(std::shared_ptr<Pathfinder::Texture> target_texture);

    void set_render_target_size(Vec2I size);

    std::shared_ptr<Node> get_tree_root() const;

    void register_fallback_font(Script script, const std::shared_ptr<Font> &font);

private:
    std::unique_ptr<SceneTree> tree;

    // Server instances
    std::unique_ptr<Engine> engine;
    std::unique_ptr<VectorServer> vector_server;
    std::unique_ptr<TextServer> text_server;
    std::unique_ptr<InputServer> input_server;
    std::unique_ptr<RenderContext> render_context;
    std::unique_ptr<DefaultResource> default_resource;
    std::unique_ptr<TranslationServer> translation_server;

    GuiContext context;
};

} // namespace vecgui
