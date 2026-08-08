#include "vecgui/app.h"

using namespace vecgui;

using Pathfinder::Vec2;
using Pathfinder::Vec3;

class MyProgressBar : public ProgressBar {
    void on_update(double dt) override {
        float new_value = value + dt * 10.0f;
        if (new_value > max_value) {
            new_value -= max_value;
        }
        set_value(new_value);
    }
};

class MyNode : public Node {
    void on_ready() override {
        auto vbox_container = std::make_shared<VBoxContainer>();
        add_child(vbox_container);

        for (int i = 0; i < 5; i++) {
            auto progress_bar = std::make_shared<MyProgressBar>();
            progress_bar->set_value(i * 20);
            vbox_container->add_child(progress_bar);
        }

        vbox_container->set_size({400, 300});
    }
};

int main() {
    App app({640, 480});

    app.get_tree_root()->add_child(std::make_shared<MyNode>());

    app.main_loop();

    return EXIT_SUCCESS;
}
