#pragma once

#include <optional>

#include "../common/geometry.h"

namespace vecgui {

struct StyleBox {
    ColorU bg_color = ColorU(27, 27, 27, 255);

    ColorU border_color = ColorU(67, 67, 67, 255);
    float border_width = 0;
    float corner_radius = 8;

    std::optional<RectF> border_widths;
    // Top-left, top-eight, bottom-left, bottom-right.
    std::optional<RectF> corner_radii;

    ColorU shadow_color;
    float shadow_size = 0;
    Vec2F shadow_offset;

    static StyleBox from_empty();

    StyleBox lerp_style_box(StyleBox target_box, float t) const;
};

struct StyleLine {
    ColorU color = ColorU(163, 163, 163, 255);

    float width = 2;
};

} // namespace vecgui
