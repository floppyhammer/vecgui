#include "panel.h"

#include <string>

#include "../../common/context.h"
#include "../../common/utils.h"
#include "../../resources/default_resource.h"
#include "../../servers/vector_server.h"

namespace vecgui {

Panel::Panel() {
    type = NodeType::Panel;
}

void Panel::draw() {
    if (!visible_) {
        return;
    }

    auto context = get_context();
    if (!context) {
        return;
    }

    auto vector_server = context->vector_server;

    auto global_position = get_global_position();

    auto default_theme = context->default_resource->get_default_theme();

    auto theme_panel = theme_override_bg_.value_or(default_theme->panel.styles["background"]);

    vector_server->draw_style_box(theme_panel, global_position, size);
}

} // namespace vecgui
