#pragma once

#include <memory>
#include "../common/geometry.h"
#include "node.h"

namespace Pathfinder {
class Texture;
}

namespace vecgui {

/**
 * A base class for anything that can be rendered into a texture.
 * In offscreen mode, this is used as the root node.
 */
class RenderTarget : public Node {
public:
    explicit RenderTarget(Vec2I size);

    void update(double dt) override;

    virtual void pre_draw_propagation();

    virtual void post_draw_propagation();

    Vec2I get_size() const;

    void set_size(Vec2I size);

    std::shared_ptr<Pathfinder::Texture> get_vector_target() const {
        return vector_target_;
    }

    void set_vector_target(std::shared_ptr<Pathfinder::Texture> texture) {
        vector_target_ = texture;
    }

    float get_dpi_scale() const {
        return dpi_scale_;
    }

    void set_dpi_scale(float scale) {
        dpi_scale_ = scale;
    }

protected:
    Vec2I size_;

    float dpi_scale_ = 1.0f;

    std::shared_ptr<Pathfinder::Texture> vector_target_;
};

} // namespace vecgui
