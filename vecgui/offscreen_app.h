#pragma once

#include <memory>

#include "common/geometry.h"
#include "nodes/node.h"
#include "nodes/scene_tree.h"

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
};

} // namespace vecgui
