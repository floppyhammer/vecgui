#include "panel.h"

#include <string>

#include "../../common/utils.h"
#include "../../resources/default_resource.h"

namespace vecgui {

Panel::Panel() {
    type = NodeType::Panel;
}

void Panel::draw() {
    if (!visible_) {
        return;
    }

    auto vector_server = VectorServer::get_singleton();

    auto global_position = get_global_position();

    auto default_theme = DefaultResource::get_singleton()->get_default_theme();

    auto theme_panel = theme_override_bg_.value_or(default_theme->panel.styles["background"]);

    vector_server->draw_style_box(theme_panel, global_position, size);
}

} // namespace vecgui
