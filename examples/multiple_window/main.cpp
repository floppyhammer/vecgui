#include <iostream>
#include <random>

#include "vecgui/app.h"
#include "vecgui/nodes/proxy_window.h"

using namespace vecgui;

/// fixme: currently not working for Vulkan.
class MyNode : public Node {
    void on_ready() override {
        auto open_window_button = std::make_shared<Button>();
        open_window_button->set_text("Show window");
        auto close_window_button = std::make_shared<Button>();
        close_window_button->set_text("Hide window");
        auto container = std::make_shared<HBoxContainer>();
        container->add_child(open_window_button);
        container->add_child(close_window_button);

        add_child(container);

        auto sub_window = std::make_shared<ProxyWindow>(Vec2I{480, 320}, -1);
        add_child(sub_window);

        auto label = std::make_shared<Label>();
        label->set_text("This is a sub-window.");
        sub_window->add_child(label);

        auto sub_window_weak = std::weak_ptr(sub_window);
        auto callback1 = [sub_window_weak] { sub_window_weak.lock()->set_visibility(true); };
        open_window_button->connect_signal("triggered", callback1);

        auto callback2 = [sub_window_weak] { sub_window_weak.lock()->set_visibility(false); };
        close_window_button->connect_signal("triggered", callback2);
    }
};

int main() {
    App app({640, 480});

    app.get_tree_root()->add_child(std::make_shared<MyNode>());

    app.main_loop();

    return EXIT_SUCCESS;
}
