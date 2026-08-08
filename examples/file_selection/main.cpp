#include <iostream>
#include <random>

#include "vecgui/app.h"
#include "vecgui/resources/default_resource.h"

using namespace vecgui;

class MyNode : public Node {
    void on_ready() override {
        auto panel = std::make_shared<Panel>();
        panel->set_anchor_flag(AnchorFlag::FullRect);
        add_child(panel);
        {
            StyleBox new_bg;
            new_bg.bg_color = ColorU(27, 27, 27, 255);
            new_bg.corner_radius = 0;
            panel->theme_override_bg_ = new_bg;
        }

        auto margin_container = std::make_shared<MarginContainer>();
        margin_container->set_margin_all(8);
        margin_container->set_anchor_flag(AnchorFlag::FullRect);
        panel->add_child(margin_container);

        auto vbox_container = std::make_shared<VBoxContainer>();
        vbox_container->set_separation(8);
        margin_container->add_child(vbox_container);

        auto label = std::make_shared<Label>();
        label->set_text("File path:");
        vbox_container->add_child(label);

        auto hbox_container = std::make_shared<HBoxContainer>();
        vbox_container->add_child(hbox_container);

        auto text_edit = std::make_shared<TextEdit>();
        text_edit->container_sizing.flag_h = ContainerSizingFlag::Fill;
        hbox_container->add_child(text_edit);

        auto file_dialog = std::make_shared<FileDialog>();
        panel->add_child(file_dialog);

        auto select_button = std::make_shared<Button>();
        select_button->set_text("Select");

        std::weak_ptr file_dialog_weak = file_dialog;
        std::weak_ptr text_edit_weak = text_edit;
        auto callback = [file_dialog_weak, text_edit_weak] {
            auto path = file_dialog_weak.lock()->show();
            if (path.has_value()) {
                text_edit_weak.lock()->set_text(path.value());
            }
        };
        select_button->connect_signal("triggered", callback);
        hbox_container->add_child(select_button);

        auto confirm_button = std::make_shared<Button>();
        confirm_button->set_text("Confirm");
        confirm_button->container_sizing.flag_h = ContainerSizingFlag::ShrinkStart;
        vbox_container->add_child(confirm_button);
    }
};

int main() {
    App app({480, 320});

    app.get_tree_root()->add_child(std::make_shared<MyNode>());

    app.main_loop();

    return EXIT_SUCCESS;
}
