#include "style_box.h"

namespace vecgui {

StyleBox StyleBox::from_empty() {
    StyleBox box;
    box.bg_color = ColorU::transparent_black();

    box.border_color = ColorU::transparent_black();
    box.border_width = 0;
    box.corner_radius = {};

    box.shadow_color = ColorU::transparent_black();
    box.shadow_size = 0;
    box.shadow_offset = Vec2F();

    return box;
}

StyleBox StyleBox::simple_outline() {
    StyleBox box;
    box.bg_color = ColorU::transparent_black();
    box.border_color = ColorU::white();
    box.corner_radius = 0;
    box.border_width = 1;
    return box;
}

StyleBox StyleBox::lerp_style_box(const StyleBox target_box, const float t) const {
    StyleBox box;
    box.border_width = Pathfinder::lerp(border_width, target_box.border_width, t);
    box.corner_radius = Pathfinder::lerp(corner_radius, target_box.corner_radius, t);

    box.bg_color = bg_color.lerp(target_box.bg_color, t);
    box.border_color = border_color.lerp(target_box.border_color, t);

    // todo: lerp corner radii
    if (corner_radii.has_value()) {
        if (target_box.corner_radii.has_value()) {
            box.corner_radii = target_box.corner_radii;
        }
    } else if (target_box.corner_radii.has_value()) {
        box.corner_radii = target_box.corner_radii;
    }

    if (border_widths.has_value()) {
        if (target_box.border_widths.has_value()) {
            box.border_widths = target_box.border_widths;
        }
    } else if (target_box.border_widths.has_value()) {
        box.border_widths = target_box.border_widths;
    }

    return box;
}

} // namespace vecgui
