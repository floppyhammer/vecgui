#pragma once

#include "theme.h"
#include "../common/context.h"

namespace vecgui {

class Font;

class DefaultResource {
public:
    DefaultResource() = default;

    void init(GuiContext* context, bool dark_mode);

    std::shared_ptr<Theme> get_default_theme() {
        return default_theme;
    }

    void set_default_theme(const std::shared_ptr<Theme> &theme) {
        default_theme = theme;
    }

    std::shared_ptr<Font> get_default_font() {
        return default_font;
    }

private:
    std::shared_ptr<Theme> default_theme;
    std::shared_ptr<Font> default_font;
};

} // namespace vecgui
