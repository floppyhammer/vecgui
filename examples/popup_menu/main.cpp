#include <iostream>
#include <random>

#include "app.h"

using namespace vecgui;

using Pathfinder::Vec2;
using Pathfinder::Vec3;

class MyNodeUi : public NodeUi {
public:
    std::weak_ptr<PopupMenu> menu_;

    void custom_ready() override {
        auto menu = std::make_shared<PopupMenu>();
        for (int i = 0; i < 20; i++) {
            menu->create_item("Item " + std::to_string(i));
        }
        menu->set_position({400, 400});
        menu->set_visibility(false);
        menu_ = menu;
        add_child(menu);
    }

    void custom_input(InputEvent &event) override {
        if (event.type == InputEventType::MouseButton) {
            auto args = event.args.mouse_button;

            if (!args.pressed && args.button == 1) {
                menu_.lock()->set_popup_position(args.position, 0);
                menu_.lock()->set_visibility(true);
            }

            if (args.pressed && args.button == 0) {
                // menu_.lock()->set_visibility(false);
            }
        }
    }
};

int main() {
    App app({640, 480});

    // Menu of a menu button.
    auto menu_button_top = std::make_shared<MenuButton>();
    menu_button_top->get_popup_menu().lock()->create_item("Item 1");
    menu_button_top->get_popup_menu().lock()->create_item("Item 2");
    menu_button_top->get_popup_menu().lock()->create_item("Item 3");
    app.get_tree_root()->add_child(menu_button_top);

    auto menu_button_bottom = std::make_shared<MenuButton>();
    menu_button_bottom->set_position({0, 250});
    menu_button_bottom->get_popup_menu().lock()->create_item("Item 1");
    menu_button_bottom->get_popup_menu().lock()->create_item("Item 2");
    menu_button_bottom->get_popup_menu().lock()->create_item("Item 3");
    app.get_tree_root()->add_child(menu_button_bottom);

    // Free menu.
    app.get_tree_root()->add_child(std::make_shared<MyNodeUi>());

    app.main_loop();

    return EXIT_SUCCESS;
}
