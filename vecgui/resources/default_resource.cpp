#include "default_resource.h"

#include "font.h"
#include "opensans_regular_ttf.h"

namespace vecgui {

void DefaultResource::init(const bool dark_mode) {
    default_theme = dark_mode ? Theme::default_dark() : Theme::default_light();

    default_font = Font::from_memory(std::vector<char>(std::begin(DEFAULT_FONT_DATA), std::end(DEFAULT_FONT_DATA)));
    assert(default_font);
}

} // namespace vecgui
