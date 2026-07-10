#pragma once

#include <optional>

#include "../common/geometry.h"
#include "node.h"

namespace vecgui {

class Blit;

class ProxyWindow : public Node {
    friend class SceneTree;

public:
    ProxyWindow(Vec2I size, int window_index);

    void update(double dt) override;

    void pre_draw_propagation();

    void post_draw_propagation();

    Vec2I get_size() const;

    void set_visibility(bool visible) override;

    std::shared_ptr<Pathfinder::Window> get_raw_window() const;

    std::shared_ptr<Pathfinder::Texture> get_vector_target() const {
        return vector_target_;
    }

    void set_vector_target(std::shared_ptr<Pathfinder::Texture> texture) {
        vector_target_ = texture;
    }

protected:
    Vec2I size_;

    std::shared_ptr<Pathfinder::Blit> blit_;

    uint8_t window_index_;

    std::shared_ptr<Pathfinder::Texture> vector_target_;

private:
    struct {
        std::shared_ptr<Pathfinder::Scene> previous_scene;
    } temp_draw_data;
};

} // namespace vecgui
