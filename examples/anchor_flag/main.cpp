#include <iostream>
#include <string>
#include <vector>

#include "vecgui/app.h"

using namespace vecgui;

class AnchorDemo : public NodeUi {
public:
    void on_ready() override {
        set_anchor_flag(AnchorFlag::FullRect);

        // 1. Create a background panel to visualize the parent area.
        auto bg = std::make_shared<Panel>();
        bg->set_anchor_flag(AnchorFlag::FullRect);
        auto bg_style = StyleBox();
        bg_style.bg_color = ColorU(40, 40, 40);
        bg->theme_override_bg_ = bg_style;
        add_child(bg);

        // 2. Create the target node that will change its anchor.
        target_node = std::make_shared<Panel>();
        target_node->set_position({100, 100});
        target_node->set_custom_minimum_size({150, 100});
        auto target_style = StyleBox();
        target_style.bg_color = ColorU(200, 50, 50, 200);
        target_style.border_color = ColorU::white();
        target_style.border_width = 2;
        target_style.corner_radius = 4;
        target_node->theme_override_bg_ = target_style;

        bg->add_child(target_node);

        // 3. Create a MenuButton to switch anchors.
        auto menu_btn = std::make_shared<MenuButton>();
        menu_btn->set_text("Select Anchor Flag");
        menu_btn->set_anchor_flag(AnchorFlag::Center);
        add_child(menu_btn);

        auto menu = menu_btn->get_popup_menu().lock();
        std::vector<std::string> flag_names = {"None",
                                               "TopLeft",
                                               "TopRight",
                                               "BottomLeft",
                                               "BottomRight",
                                               "CenterLeft",
                                               "CenterRight",
                                               "CenterTop",
                                               "CenterBottom",
                                               "Center",
                                               "LeftWide",
                                               "RightWide",
                                               "TopWide",
                                               "BottomWide",
                                               "VCenterWide",
                                               "HCenterWide",
                                               "FullRect"};

        for (const auto& name : flag_names) {
            menu->create_item(name);
        }

        menu->connect_signal("item_selected", [this, flag_names](uint32_t index) {
            AnchorFlag selected = (AnchorFlag)index;
            target_node->set_anchor_flag(selected);
            std::cout << "Switched to anchor: " << flag_names[index] << std::endl;
        });

        // Help text
        auto help = std::make_shared<Label>();
        help->set_text(
            "Use the menu to change the red panel's AnchorFlag.\nNotice how 'Wide' and 'FullRect' expand the size.");
        help->set_anchor_flag(AnchorFlag::BottomWide);
        add_child(help);
    }

private:
    std::shared_ptr<Panel> target_node;
};

int main() {
    App app({800, 600});
    app.set_window_title("Anchor Flag Demo");

    auto root = app.get_tree_root();
    root->add_child(std::make_shared<AnchorDemo>());

    app.main_loop();

    return 0;
}
