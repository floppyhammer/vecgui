#include <iostream>
#include <random>

#include "app.h"

using namespace vecgui;

using Pathfinder::Vec2;
using Pathfinder::Vec3;

class MyNode : public Node {
    std::shared_ptr<HBoxContainer> water_fill_hbox;
    float timer = 0;

    void on_ready() override {
        auto hbox_container = std::make_shared<HBoxContainer>();
        hbox_container->set_separation(8);
        hbox_container->set_position({100, 100});
        hbox_container->theme_override_bg = StyleBox();
        add_child(hbox_container);

        for (int _ = 0; _ < 4; _++) {
            auto button = std::make_shared<Button>();
            hbox_container->add_child(button);

            if (_ == 0) {
                button->container_sizing.flag_h = ContainerSizingFlag::ShrinkCenter;
                button->container_sizing.flag_v = ContainerSizingFlag::Fill;
            }
            if (_ == 1) {
                button->container_sizing.flag_v = ContainerSizingFlag::ShrinkStart;
            }
            if (_ == 2) {
                button->container_sizing.flag_v = ContainerSizingFlag::ShrinkCenter;
            }
            if (_ == 3) {
                button->container_sizing.flag_v = ContainerSizingFlag::ShrinkEnd;
            }
        }
        hbox_container->set_size({800, 100});

        auto vbox_container = std::make_shared<VBoxContainer>();
        vbox_container->set_separation(8);
        vbox_container->set_position({100, 300});
        vbox_container->theme_override_bg = StyleBox();
        add_child(vbox_container);

        for (int _ = 0; _ < 4; _++) {
            auto button = std::make_shared<Button>();
            vbox_container->add_child(button);

            if (_ == 0) {
                button->container_sizing.flag_v = ContainerSizingFlag::ShrinkCenter;
                button->container_sizing.flag_h = ContainerSizingFlag::Fill;
            }
            if (_ == 1) {
                button->container_sizing.flag_h = ContainerSizingFlag::ShrinkStart;
            }
            if (_ == 2) {
                button->container_sizing.flag_h = ContainerSizingFlag::ShrinkCenter;
            }
            if (_ == 3) {
                button->container_sizing.flag_h = ContainerSizingFlag::ShrinkEnd;
            }
        }
        vbox_container->set_size({200, 300});

        // BoxContainer alignment.
        {
            auto box_container = std::make_shared<VBoxContainer>();
            box_container->set_separation(8);
            box_container->set_position({350, 400});
            box_container->theme_override_bg = StyleBox();
            box_container->set_alignment(BoxContainerAlignment::End);
            add_child(box_container);

            for (int i = 0; i < 3; i++) {
                auto label = std::make_shared<Label>();
                box_container->add_child(label);
                label->set_text("Label " + std::to_string(i));
            }
            box_container->set_size({100, 200});
        }

        auto grid_container = std::make_shared<GridContainer>();
        grid_container->set_separation(8);
        grid_container->set_position({600, 400});
        grid_container->set_column_limit(2);
        grid_container->theme_override_bg = StyleBox();
        grid_container->set_item_shrinking(false);
        add_child(grid_container);

        for (int _ = 0; _ < 4; _++) {
            auto button = std::make_shared<Button>();
            grid_container->add_child(button);
        }
        grid_container->set_size({200, 300});

        // --- Water-filling Test ---
        {
            auto label = std::make_shared<Label>();
            label->set_text("Water-filling: B1(min 50), B2(min 150), B3(min 100)");
            label->set_position({350, 230});
            add_child(label);

            auto hbox = std::make_shared<HBoxContainer>();
            hbox->set_separation(10);
            hbox->set_position({350, 260});
            hbox->theme_override_bg = StyleBox();
            add_child(hbox);

            auto b1 = std::make_shared<Button>();
            b1->set_text("B1");
            b1->set_custom_minimum_size({50, 40});
            b1->container_sizing.flag_h = ContainerSizingFlag::Fill;
            hbox->add_child(b1);

            auto b2 = std::make_shared<Button>();
            b2->set_text("B2");
            b2->set_custom_minimum_size({150, 40});
            b2->container_sizing.flag_h = ContainerSizingFlag::Fill;
            hbox->add_child(b2);

            auto b3 = std::make_shared<Button>();
            b3->set_text("B3");
            b3->set_custom_minimum_size({100, 40});
            b3->container_sizing.flag_h = ContainerSizingFlag::Fill;
            hbox->add_child(b3);

            // Total min width = 320. Set to 400 (80 extra).
            // Expect: B1=115, B3=115, B2=150.
            hbox->set_size({400, 60});
            water_fill_hbox = hbox;
        }
    }

    void on_update(double dt) override {
        if (water_fill_hbox) {
            timer += (float)dt;
            // 让宽度在 320 (最小宽度) 到 700 之间循环振荡
            float dynamic_width = 320.0f + (std::sin(timer * 1.5f) + 1.0f) * 0.5f * 380.0f;
            water_fill_hbox->set_size({dynamic_width, 60});
        }
    }
};

int main() {
    App app({1280, 720});

    app.get_tree_root()->add_child(std::make_shared<MyNode>());

    app.main_loop();

    return EXIT_SUCCESS;
}
