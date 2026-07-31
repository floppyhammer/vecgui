#pragma once

#include <map>

#include "style_box.h"

namespace vecgui {

class Font;

class Theme {
public:
    Theme() = default;

    static std::shared_ptr<Theme> default_dark();

    static std::shared_ptr<Theme> default_light();

    // TODO
    static std::shared_ptr<Theme> from_json(const std::string& json);

    void load_font(const std::string& font_path);

    std::shared_ptr<Font> font;

    uint32_t font_size = 24;

    ColorU accent_color;

    struct {
        std::map<std::string, float> constants;
        std::map<std::string, ColorU> colors;
        std::map<std::string, StyleBox> styles;
    } button;

    struct {
        std::map<std::string, float> constants;
        std::map<std::string, ColorU> colors;
        std::map<std::string, StyleBox> styles;
    } label;

    struct {
        std::map<std::string, ColorU> colors;
        std::map<std::string, StyleBox> styles;
    } panel;

    struct {
        std::map<std::string, ColorU> colors;
        std::map<std::string, StyleBox> styles;
    } tab_container;

    struct {
        std::map<std::string, ColorU> colors;
        std::map<std::string, StyleBox> styles;
    } collapse_container;

    struct {
        std::map<std::string, ColorU> colors;
        std::map<std::string, StyleBox> styles;
    } popup_menu;

    struct {
        std::map<std::string, ColorU> colors;
        std::map<std::string, StyleBox> styles;
        StyleLine caret;
    } text_edit;

    struct {
        std::map<std::string, ColorU> colors;
    } slider;
};

} // namespace vecgui
