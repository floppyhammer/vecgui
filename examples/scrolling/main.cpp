#include "app.h"

using namespace vecgui;

using Pathfinder::Vec2;
using Pathfinder::Vec3;

class MyNode : public Node {
    void custom_ready() override {
        auto panel = std::make_shared<Panel>();
        panel->set_position({100, 100});
        panel->set_size({200, 300});
        add_child(panel);

        auto margin_container = std::make_shared<MarginContainer>();
        margin_container->set_anchor_flag(AnchorFlag::FullRect);
        panel->add_child(margin_container);

        auto scroll_container = std::make_shared<ScrollContainer>();
        margin_container->add_child(scroll_container);

        auto vbox_container = std::make_shared<VBoxContainer>();
        vbox_container->set_separation(8);
        scroll_container->add_child(vbox_container);

        for (int i = 0; i < 20; i++) {
            auto button = std::make_shared<Button>();
            button->container_sizing.flag_h = ContainerSizingFlag::Fill;
            vbox_container->add_child(button);
        }
    }
};

int main() {
    App app({640, 480});

    app.get_tree_root()->add_child(std::make_shared<MyNode>());

    app.main_loop();

    return EXIT_SUCCESS;
}
