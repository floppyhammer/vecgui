#pragma once

#include <list>
#include <string>
#include <unordered_map>

#include "../resources/font.h"

namespace vecgui {

class TextServer {
public:
    TextServer() = default;

    void cleanup();

    void register_fallback_font(Script script, const std::shared_ptr<Font> &font);

    void register_font(const std::string &font_name, const std::shared_ptr<Font> &font);

    bool has_registered_font(const std::string &font_name);

    void register_emoji_font(const std::shared_ptr<Font> &font);

    std::shared_ptr<Font> get_font_for_script(Script script);

    std::shared_ptr<Font> get_font(const std::string &font_name);

    std::shared_ptr<Font> get_emoji_font();

private:
    std::unordered_map<Script, std::shared_ptr<Font>> script_fallback_map;

    std::unordered_map<std::string, std::shared_ptr<Font>> registered_fonts;

    std::list<std::string> font_order;

    std::shared_ptr<Font> emoji_font;

    static constexpr size_t MAX_FONT_CACHE_SIZE = 10;
};

} // namespace vecgui
