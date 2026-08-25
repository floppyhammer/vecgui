#include "vecgui/app.h"

using namespace vecgui;

using Pathfinder::Vec2;
using Pathfinder::Vec3;

class MyNode : public Node {
    void on_ready() override {
        auto split_container = std::make_shared<SplitContainer>();
        split_container->set_position({100, 100});
        add_child(split_container);

        for (int _ = 0; _ < 2; _++) {
            auto button = std::make_shared<Button>();
            split_container->add_child(button);
        }
        split_container->set_size({800, 100});
    }
};

int main() {
    App app({1280, 720});

    app.get_tree_root()->add_child(std::make_shared<MyNode>());

    app.main_loop();

    return EXIT_SUCCESS;
}
