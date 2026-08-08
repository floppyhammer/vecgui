#pragma once

#include <memory>

#include "node_ui.h"

namespace vecgui {

/**
 * Used to display an ImageTexture or a VectorTexture.
 */
class TextureRect : public NodeUi {
public:
    TextureRect();

    void set_texture(const std::shared_ptr<Image> &new_texture);

    std::shared_ptr<Image> get_texture() const;

    void calc_minimum_size() override;

    enum class StretchMode {
        Scale,        // Scale it to the TextureRect size.
        Keep,         // Do nothing.
        KeepCentered, // Center it but do not scale it.
        KeepAspect,
        KeepAspectCentered,
        KeepCovered, // Not available for VectorTexture.
        Tile,        // Do not work.
        Max,
    };

    void set_stretch_mode(StretchMode new_mode);

    void draw() override;

protected:
    void update(double dt) override;

    StretchMode stretch_mode = StretchMode::Scale;

    std::shared_ptr<Image> texture;
};

} // namespace vecgui
