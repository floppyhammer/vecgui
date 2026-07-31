#include <iostream>
#include <random>

#include "app.h"

using namespace vecgui;

class MyNode : public NodeUi {
    void custom_ready() override {
        set_anchor_flag(AnchorFlag::FullRect);

        auto slider1 = std::make_shared<Slider>();
        slider1->set_position({200, 200});
        slider1->set_size({400, 40});
        slider1->set_range(1, 10);
        add_child(slider1);

        auto callback1 = [](float value) { std::cout << value << std::endl; };
        slider1->connect_signal("value_changed", callback1);

        auto slider2 = std::make_shared<Slider>();
        slider2->set_position({200, 300});
        slider2->set_size({400, 40});
        slider2->set_range(1, 10);
        slider2->set_integer_mode(true);
        add_child(slider2);

        auto callback2 = [](float value) { std::cout << value << std::endl; };
        slider2->connect_signal("value_changed", callback2);
    }
};

int main() {
    App app({960, 480});

    app.get_tree_root()->add_child(std::make_shared<MyNode>());

    app.main_loop();

    return EXIT_SUCCESS;
}
