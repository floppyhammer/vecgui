#include <random>

#include "vecgui/app.h"

using namespace vecgui;

using Pathfinder::Vec2;
using Pathfinder::Vec3;

class MyNode : public Node {
    void on_ready() override {
        auto collapse = std::make_shared<CollapseContainer>(CollapseButtonType::Default);
        collapse->set_position({100, 100});
        collapse->set_size({400, 300});
        add_child(collapse);

        auto tree = std::make_shared<Tree>();
        tree->set_anchor_flag(AnchorFlag::FullRect);
        collapse->add_child(tree);

        // Set up tree items.
        {
            auto context = get_context();
            auto tree_root = tree->create_item(nullptr, "Node");
            tree_root->set_icon(std::make_shared<VectorImage>(context, get_asset_dir("icons/Node_Node.svg")));
            auto child_ui = tree->create_item(tree_root, "NodeUi");
            child_ui->set_icon(std::make_shared<VectorImage>(context, get_asset_dir("icons/Node_Control.svg")));
            auto child_label = tree->create_item(child_ui, "Label");
            child_label->set_icon(std::make_shared<VectorImage>(context, get_asset_dir("icons/Node_Label.svg")));
            auto child_text_edit = tree->create_item(child_ui, "TextEdit");
            child_text_edit->set_icon(std::make_shared<VectorImage>(context, get_asset_dir("icons/Node_LineEdit.svg")));
            auto child_node_2d = tree->create_item(tree_root, "Node2d");
            child_node_2d->set_icon(std::make_shared<VectorImage>(context, get_asset_dir("icons/Node_Node2D.svg")));
            auto child_node_3d = tree->create_item(tree_root, "Node3d");
            child_node_3d->set_icon(std::make_shared<VectorImage>(context, get_asset_dir("icons/Node_Node3D.svg")));
        }
    }
};

int main() {
    App app({640, 480});

    app.get_tree_root()->add_child(std::make_shared<MyNode>());

    app.main_loop();

    return EXIT_SUCCESS;
}
