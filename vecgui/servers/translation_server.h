#pragma once

#include <pathfinder/prelude.h>

#include <vector>

namespace vecgui {

class TranslationServer {
public:
    TranslationServer();

    void set_locale(const std::string &locale);

    std::string get_translation(std::string tag);

    void load_translations(const std::string& filename);

private:
    std::map<std::string, std::map<std::string, std::string>> db_;

    std::string current_locale_ = "en";
};

} // namespace vecgui
