#include "text_server.h"

namespace vecgui {

void TextServer::cleanup() {
    script_fallback_map.clear();
    registered_fonts.clear();
    font_order.clear();
}

void TextServer::register_fallback_font(Script script, const std::shared_ptr<Font> &font) {
    if (!font) {
        return;
    }
    script_fallback_map[script] = font;
}

void TextServer::register_font(const std::string &font_name, const std::shared_ptr<Font> &font) {
    if (!font || font_name.empty()) {
        return;
    }

    auto it = registered_fonts.find(font_name);
    if (it != registered_fonts.end()) {
        // Font name already exists, update the font and refresh its position in the registration order.
        font_order.remove(font_name);
    } else {
        // Check if we need to evict the oldest registered font.
        if (registered_fonts.size() >= MAX_FONT_CACHE_SIZE) {
            std::string oldest = font_order.front();
            font_order.pop_front();
            registered_fonts.erase(oldest);
        }
    }

    registered_fonts[font_name] = font;
    font_order.push_back(font_name);
}

bool TextServer::has_registered_font(const std::string &font_name) {
    return registered_fonts.find(font_name) != registered_fonts.end();
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

std::shared_ptr<Font> TextServer::get_font(const std::string &font_name) {
    auto it = registered_fonts.find(font_name);
    if (it != registered_fonts.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<Font> TextServer::get_emoji_font() {
    return emoji_font;
}

} // namespace vecgui
