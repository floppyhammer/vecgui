#pragma once

#include <string>
#include <unordered_map>

#include "../resources/font.h"

namespace vecgui {

class TextServer {
public:
    std::shared_ptr<std::vector<char>> get_font(std::string font_id);

    void cleanup();

private:
    std::string clipboard;

    // Cache for raw font data.
    std::unordered_map<std::string, std::shared_ptr<std::vector<char>>> raw_font_cache;

    std::unordered_map<std::string, std::shared_ptr<Font>> font_cache;
};

} // namespace vecgui
