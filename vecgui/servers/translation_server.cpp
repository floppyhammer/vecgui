#include "translation_server.h"

#include <pathfinder/prelude.h>
#include <rapidcsv.h>

#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace vecgui {

TranslationServer::TranslationServer() = default;

void TranslationServer::set_locale(const std::string& locale) {
    current_locale_ = locale;
}

std::string TranslationServer::get_translation(std::string tag) {
    const auto& locale_map = db_[current_locale_];

    // Fallback
    if (!locale_map.contains(tag)) {
        return tag;
    }

    return locale_map.at(tag);
}

void TranslationServer::load_translations(const std::string& filename) {
    std::ifstream file(filename);

    rapidcsv::Document doc(filename);
    auto row_count = doc.GetRowCount();
    auto locale_count = doc.GetColumnCount() - 1;
    auto locale_names = doc.GetColumnNames();

    for (size_t row_idx = 0; row_idx < row_count; row_idx++) {
        auto row = doc.GetRow<std::string>(row_idx);

        for (int i = 0; i < locale_count; i++) {
            const auto& locale = locale_names[1 + i];
            const auto& tag = row[0];

            db_[locale][tag] = row[1 + i];
        }
    }
}

} // namespace vecgui
