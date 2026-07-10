#include "theme.h"

#include "default_resource.h"
#include "font.h"

namespace vecgui {

std::shared_ptr<Theme> Theme::default_dark() {
    auto theme = std::make_shared<Theme>();

    theme->accent_color = ColorU(70, 150, 235);

    // Button
    {
        theme->button.colors["text"] = ColorU(200, 200, 200);
        theme->button.colors["text_disabled"] = ColorU(200, 200, 200, 100);

        auto style_box_normal = StyleBox();
        style_box_normal.bg_color = ColorU(32, 32, 32);
        style_box_normal.border_color = ColorU(67, 67, 67);
        style_box_normal.border_width = 2;
        style_box_normal.corner_radius = 8;
        theme->button.styles["normal"] = style_box_normal;

        {
            auto style_box = style_box_normal;
            style_box.bg_color = ColorU(41, 41, 41);
            theme->button.styles["hovered"] = style_box;
        }

        {
            auto style_box = style_box_normal;
            style_box.bg_color = ColorU(45, 45, 45);
            style_box.border_color = theme->accent_color;
            theme->button.styles["pressed"] = style_box;
        }

        {
            auto style_box = style_box_normal;
            style_box.bg_color = ColorU(32, 32, 32, 100);
            style_box.border_width = 0;
            style_box.corner_radius = 0;
            style_box.border_color = ColorU(67, 67, 67, 150);
            theme->button.styles["disabled"] = style_box;
        }
    }

    // Label
    theme->label.colors["text"] = ColorU(200, 200, 200);
    theme->label.styles["background"] = StyleBox::from_empty();

    // Panel
    {
        auto style_box = StyleBox();
        style_box.bg_color = ColorU(32, 32, 32, 255);
        style_box.border_color = {75, 75, 75, 100};
        style_box.border_width = 2;
        style_box.corner_radius = 8;
        theme->panel.styles["background"] = style_box;
    }

    // Collapse container
    {
        {
            auto style_box = StyleBox();
            style_box.bg_color = ColorU(86, 170, 114, 255);
            style_box.border_width = 0;
            style_box.corner_radius = 8;
            theme->collapse_container.styles["title_bar"] = style_box;
        }

        {
            auto style_box = StyleBox();
            style_box.bg_color = ColorU(32, 32, 32, 255);
            style_box.border_color = {86, 170, 114, 255};
            style_box.border_width = 2;
            style_box.corner_radius = 8;
            theme->collapse_container.styles["background"] = style_box;
        }
    }

    // Tab container
    {
        {
            auto style_box = StyleBox();
            style_box.corner_radius = 0;
            style_box.border_width = 0;
            style_box.bg_color = ColorU(32, 32, 32);
            theme->tab_container.styles["background"] = style_box;
        }
        {
            auto style_box = StyleBox();
            style_box.corner_radius = 0;
            style_box.border_width = 0;
            style_box.bg_color = ColorU(40, 40, 40);
            theme->tab_container.styles["tab_background"] = style_box;
        }
        {
            auto style_box = StyleBox();
            style_box.border_width = 0;
            style_box.corner_radius = 0;
            style_box.bg_color = ColorU(40, 40, 40);
            theme->tab_container.styles["tab_button_normal"] = style_box;
        }

        {
            auto style_box = StyleBox();
            style_box.border_width = 0;
            style_box.corner_radius = 0;
            style_box.bg_color = ColorU(32, 32, 32);
            style_box.border_widths = {0, 4, 0, 0};
            style_box.border_color = theme->accent_color;
            theme->tab_container.styles["tab_button_pressed"] = style_box;
        }
    }

    // Popup menu
    {
        {
            auto style_box = StyleBox();
            style_box.bg_color = ColorU(32, 32, 32, 255);
            style_box.border_color = {75, 75, 75, 100};
            style_box.border_width = 2;
            style_box.corner_radius = 8;
            theme->popup_menu.styles["background"] = style_box;
        }

        {
            auto style_box = StyleBox();
            style_box.bg_color = ColorU(255, 255, 255, 0);
            style_box.border_width = 0;
            theme->popup_menu.styles["item_normal"] = style_box;
        }

        {
            auto style_box = StyleBox();
            style_box.bg_color = theme->accent_color;
            style_box.border_width = 0;
            theme->popup_menu.styles["item_hovered"] = style_box;
        }
    }

    // Text edit
    {
        {
            auto style_box = StyleBox();
            style_box.corner_radius = 8;
            style_box.border_width = 0;
            style_box.bg_color = ColorU(10, 10, 10);
            theme->text_edit.styles["normal"] = style_box;
        }

        {
            auto style_box = StyleBox();
            style_box.corner_radius = 8;
            style_box.border_width = 2;
            style_box.border_color = theme->accent_color;
            style_box.bg_color = ColorU(10, 10, 10);
            theme->text_edit.styles["focused"] = style_box;
        }

        theme->text_edit.caret.color = ColorU(200, 200, 200);
        theme->text_edit.caret.width = 1.5;

        {
            auto style_box = StyleBox();
            style_box.border_width = 0;
            style_box.corner_radius = 0;
            style_box.bg_color = theme->accent_color;
            theme->text_edit.styles["selection"] = style_box;
        }
    }

    // Slider
    {
        theme->slider.colors["unfilled"] = ColorU(85, 85, 85);
        theme->slider.colors["filled"] = theme->accent_color;
        theme->slider.colors["grabber_border"] = theme->accent_color;
        theme->slider.colors["grabber_fill"] = ColorU(200, 200, 200);
        theme->slider.colors["grabber_fill_hovered"] = ColorU(235, 235, 235);
        theme->slider.colors["grabber_fill_pressed"] = ColorU(235, 235, 235, 150);
    }

    theme->load_unifont();

    return theme;
}

std::shared_ptr<Theme> Theme::default_light() {
    auto theme = std::make_shared<Theme>();

    theme->accent_color = ColorU(90, 170, 255);

    theme->button.colors["text"] = ColorU(55, 55, 55);
    theme->button.colors["text_disabled"] = ColorU(55, 55, 55, 100);

    auto style_box_normal = StyleBox();
    style_box_normal.bg_color = ColorU(223, 223, 223);
    style_box_normal.border_color = ColorU(188, 188, 188);
    style_box_normal.border_width = 2;
    style_box_normal.corner_radius = 8;
    theme->button.styles["normal"] = style_box_normal;

    {
        auto style_box = style_box_normal;
        style_box.bg_color = ColorU(210, 210, 210);
        theme->button.styles["hovered"] = style_box;
    }

    {
        auto style_box = style_box_normal;
        style_box.bg_color = ColorU(210, 210, 210, 255);
        style_box.border_color = theme->accent_color;
        theme->button.styles["pressed"] = style_box;
    }

    {
        auto style_box = style_box_normal;
        style_box.bg_color = ColorU(223, 223, 223, 100);
        style_box.border_width = 0;
        style_box.corner_radius = 0;
        style_box.border_color = ColorU(188, 188, 188, 150);
        theme->button.styles["disabled"] = style_box;
    }

    theme->label.colors["text"] = ColorU(55, 55, 55);
    theme->label.styles["background"] = StyleBox::from_empty();

    // Panel
    {
        auto style_box = StyleBox();
        style_box.bg_color = ColorU(223, 223, 223);
        style_box.border_color = {180, 180, 180, 100};
        style_box.border_width = 2;
        style_box.corner_radius = 8;
        theme->panel.styles["background"] = style_box;
    }

    {
        {
            auto style_box = StyleBox();
            style_box.bg_color = ColorU(86, 170, 114, 255);
            style_box.border_width = 0;
            style_box.corner_radius = 8;
            theme->collapse_container.styles["title_bar"] = style_box;
        }

        {
            auto style_box = StyleBox();
            style_box.bg_color = ColorU(223, 223, 223, 255);
            style_box.border_color = {86, 170, 114, 255};
            style_box.border_width = 2;
            style_box.corner_radius = 8;
            theme->collapse_container.styles["background"] = style_box;
        }
    }

    // Tab container
    {
        {
            auto style_box = StyleBox();
            style_box.corner_radius = 0;
            style_box.border_width = 0;
            style_box.bg_color = ColorU(200, 200, 200);
            theme->tab_container.styles["background"] = style_box;
        }
        {
            auto style_box = StyleBox();
            style_box.corner_radius = 0;
            style_box.border_width = 0;
            style_box.bg_color = ColorU(230, 230, 230);
            theme->tab_container.styles["tab_background"] = style_box;
        }
        {
            auto style_box = StyleBox();
            style_box.border_width = 0;
            style_box.corner_radius = 0;
            style_box.bg_color = ColorU(230, 230, 230);
            theme->tab_container.styles["tab_button_normal"] = style_box;
        }

        {
            auto style_box = StyleBox();
            style_box.border_width = 0;
            style_box.corner_radius = 0;
            style_box.bg_color = ColorU(200, 200, 200);
            style_box.border_widths = {0, 4, 0, 0};
            style_box.border_color = theme->accent_color;
            theme->tab_container.styles["tab_button_pressed"] = style_box;
        }
    }

    // Text edit
    {
        {
            auto style_box = StyleBox();
            style_box.border_width = 0;
            style_box.bg_color = ColorU(232, 232, 232);
            theme->text_edit.styles["normal"] = style_box;
        }

        {
            auto style_box = StyleBox();
            style_box.corner_radius = 8;
            style_box.border_width = 2;
            style_box.border_color = theme->accent_color;
            style_box.bg_color = ColorU(232, 232, 232);
            theme->text_edit.styles["focused"] = style_box;
        }

        theme->text_edit.caret.color = ColorU(55, 55, 55);
        theme->text_edit.caret.width = 1.5;

        {
            auto style_box = StyleBox();
            style_box.border_width = 0;
            style_box.corner_radius = 0;
            style_box.bg_color = theme->accent_color;
            theme->text_edit.styles["selection"] = style_box;
        }
    }

    // Popup menu
    {
        {
            auto style_box = StyleBox();
            style_box.bg_color = ColorU(223, 223, 223, 255);
            style_box.border_color = {180, 180, 180, 100};
            style_box.border_width = 2;
            style_box.corner_radius = 8;
            theme->popup_menu.styles["background"] = style_box;
        }

        {
            auto style_box = StyleBox();
            style_box.bg_color = ColorU(255, 255, 255, 0);
            style_box.border_width = 0;
            theme->popup_menu.styles["item_normal"] = style_box;
        }

        {
            auto style_box = StyleBox();
            style_box.bg_color = theme->accent_color;
            style_box.border_width = 0;
            theme->popup_menu.styles["item_hovered"] = style_box;
        }
    }

    // Slider
    {
        theme->slider.colors["unfilled"] = ColorU(170, 170, 170);
        theme->slider.colors["filled"] = theme->accent_color;
        theme->slider.colors["grabber_border"] = theme->accent_color;
        theme->slider.colors["grabber_fill"] = ColorU(220, 220, 220);
        theme->slider.colors["grabber_fill_hovered"] = ColorU(255, 255, 255);
        theme->slider.colors["grabber_fill_pressed"] = ColorU(255, 255, 255, 150);
    }

    theme->load_unifont();

    return theme;
}

std::shared_ptr<Theme> Theme::from_json(const std::string& json) {
    return nullptr;
}

void Theme::load_unifont() {
    font = DefaultResource::get_singleton()->get_default_font();

    const auto new_font = Font::from_file(get_asset_dir("unifont-17.0.03.otf"));
    if (new_font) {
        font = new_font;
    } else {
        Logger::warn("Unifont not found", "vecgui");
    }
}

} // namespace vecgui
