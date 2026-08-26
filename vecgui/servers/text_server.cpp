#include "text_server.h"

namespace vecgui {

void TextServer::cleanup() {
    script_fallback_map.clear();
}

void TextServer::register_fallback_font(Script script, const std::shared_ptr<Font> &font) {
    if (!font) {
        return;
    }
    script_fallback_map[script] = font;
}

void TextServer::register_emoji_font(const std::shared_ptr<Font> &font) {
    if (!font) {
        return;
    }
    emoji_font = font;
}

std::shared_ptr<Font> TextServer::get_font_for_script(Script script) {
    auto find = script_fallback_map.find(script);
    if (find != script_fallback_map.end()) {
        return find->second;
    }
    return nullptr;
}

std::shared_ptr<Font> TextServer::get_emoji_font() {
    return emoji_font;
}

} // namespace vecgui
