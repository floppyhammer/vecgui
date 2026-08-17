#pragma once

#include <string>
#include <unordered_map>

#include "../resources/font.h"

namespace vecgui {

class TextServer {
public:
    TextServer() = default;

    void cleanup();

    void register_fallback_font(Script script, const std::shared_ptr<Font> &font);

    std::shared_ptr<Font> get_font_for_script(Script script);

private:
    std::unordered_map<Script, std::shared_ptr<Font>> script_fallback_map;
};

} // namespace vecgui
