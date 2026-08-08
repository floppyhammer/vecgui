#include <iostream>
#include <random>

#include "app.h"

using namespace vecgui;

using Pathfinder::Vec2;
using Pathfinder::Vec3;

class MyNode : public Node {
    std::shared_ptr<ToggleButtonGroup> button_group;

    void on_ready() override {
        auto vbox_container = std::make_shared<VBoxContainer>();
        vbox_container->set_separation(8);
        vbox_container->set_position({100, 100});
        add_child(vbox_container);

        {
            auto button = std::make_shared<Button>();
            button->container_sizing.flag_h = ContainerSizingFlag::ShrinkStart;
            vbox_container->add_child(button);

            auto callback = []() { Logger::info("Button triggered"); };
            button->connect_signal("triggered", callback);
        }

        {
            auto button = std::make_shared<Button>();
            button->set_icon_normal(std::make_shared<VectorImage>(get_asset_dir("icons/Node_Button.svg")));
            button->container_sizing.flag_h = ContainerSizingFlag::ShrinkStart;
            vbox_container->add_child(button);
        }

        {
            auto check_button = std::make_shared<CheckButton>();
            check_button->container_sizing.flag_h = ContainerSizingFlag::ShrinkStart;
            vbox_container->add_child(check_button);

            auto callback = [](bool toggled) { Logger::info("Button toggled"); };
            check_button->connect_signal("toggled", callback);
        }

        {
            auto container_group = std::make_shared<VBoxContainer>();
            container_group->set_separation(8);
            container_group->set_position({300, 100});
            add_child(container_group);

            auto label = std::make_shared<Label>();
            label->set_text("Toggle Button Group");
            container_group->add_child(label);

            button_group = std::make_shared<ToggleButtonGroup>();

            for (int i = 0; i < 3; ++i) {
                auto check_button = std::make_shared<RadioButton>();
                check_button->container_sizing.flag_h = ContainerSizingFlag::ShrinkStart;
                container_group->add_child(check_button);
                button_group->add_button(check_button);
            }
        }
    }
};

int main() {
    App app({960, 480});

    app.get_tree_root()->add_child(std::make_shared<MyNode>());

    app.main_loop();

    return EXIT_SUCCESS;
}
