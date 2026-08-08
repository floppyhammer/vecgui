#include <random>

#include "vecgui/app.h"

using namespace vecgui;

using Pathfinder::Vec2;
using Pathfinder::Vec3;

class MyNode : public Node {
    void on_ready() override {
        auto vbox_container = std::make_shared<VBoxContainer>();
        add_child(vbox_container);
        vbox_container->set_anchor_flag(AnchorFlag::CenterLeft);
        vbox_container->set_separation(8);

        Vec2F label_size = {256, 64};

        {
            auto label = std::make_shared<Label>();
            label->set_text("Alignment");
            label->set_bidi_alignment(BidiAlignment::Center);
            label->set_custom_minimum_size(label_size);
            label->set_horizontal_alignment(Alignment::Begin);
            label->theme_override_bg = StyleBox();
            vbox_container->add_child(label);
        }

        {
            auto label = std::make_shared<Label>();
            label->set_text("Alignment");
            label->set_custom_minimum_size(label_size);
            label->set_horizontal_alignment(Alignment::Center);
            label->theme_override_bg = StyleBox();
            vbox_container->add_child(label);
        }

        {
            auto label = std::make_shared<Label>();
            label->set_text("Alignment");
            label->set_custom_minimum_size(label_size);
            label->set_horizontal_alignment(Alignment::End);
            label->theme_override_bg = StyleBox();
            vbox_container->add_child(label);
        }

        {
            auto label = std::make_shared<Label>();
            label->set_text("Alignment");
            label->set_vertical_alignment(Alignment::Begin);
            label->set_custom_minimum_size(label_size);
            label->theme_override_bg = StyleBox();
            vbox_container->add_child(label);
        }

        {
            auto label = std::make_shared<Label>();
            label->set_text("Alignment");
            label->set_vertical_alignment(Alignment::Center);
            label->set_custom_minimum_size(label_size);
            label->theme_override_bg = StyleBox();
            vbox_container->add_child(label);
        }

        {
            auto label = std::make_shared<Label>();
            label->set_text("Alignment");
            label->set_vertical_alignment(Alignment::End);
            label->set_custom_minimum_size(label_size);
            label->theme_override_bg = StyleBox();
            vbox_container->add_child(label);
        }

        // Bidi alignment.
        {
            auto vbox_container2 = std::make_shared<VBoxContainer>();
            add_child(vbox_container2);
            vbox_container2->set_anchor_flag(AnchorFlag::CenterRight);
            vbox_container2->set_separation(8);

            {
                auto label = std::make_shared<Label>();
                label->set_text("Bidi\nAlignment");
                label->set_bidi_alignment(BidiAlignment::Begin);
                label->set_custom_minimum_size(label_size);
                label->theme_override_bg = StyleBox();
                vbox_container2->add_child(label);
            }
            {
                auto label = std::make_shared<Label>();
                label->set_text("Bidi\nAlignment");
                label->set_bidi_alignment(BidiAlignment::Center);
                label->set_custom_minimum_size(label_size);
                label->theme_override_bg = StyleBox();
                vbox_container2->add_child(label);
            }
            {
                auto label = std::make_shared<Label>();
                label->set_text("Bidi\nAlignment");
                label->set_bidi_alignment(BidiAlignment::End);
                label->set_custom_minimum_size(label_size);
                label->theme_override_bg = StyleBox();
                vbox_container2->add_child(label);
            }
        }
    }
};

int main() {
    App app({640, 480});

    app.get_tree_root()->add_child(std::make_shared<MyNode>());

    app.main_loop();

    return EXIT_SUCCESS;
}
