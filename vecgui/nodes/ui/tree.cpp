#include "tree.h"

#include <string>

#include "../../common/context.h"
#include "../../common/utils.h"
#include "../../servers/vector_server.h"
#include "../scene_tree.h"

namespace vecgui {

Tree::Tree() {
    type = NodeType::Tree;

    auto panel = StyleBox();
    panel.bg_color = ColorU(0, 0, 0, 0);
    panel.corner_radius = 4;
    theme_bg = std::make_optional(panel);
    panel.border_width = 2;
    theme_bg_focused = std::make_optional(panel);
}

void Tree::update(double dt) {
    NodeUi::update(dt);
}

void Tree::draw() {
    if (!visible_) {
        return;
    }

    auto context = get_context();
    if (!context) {
        return;
    }

    auto vector_server = context->vector_server;

    if (theme_bg.has_value()) {
        vector_server->draw_style_box(theme_bg.value(), get_global_position(), size);
    }

    float offset_y = 0;
    root->propagate_draw_(folding_width, 0, offset_y, get_global_position());
}

void Tree::input(InputEvent &event) {
    root->propagate_input(event, get_global_position());

    NodeUi::input(event);
}

std::shared_ptr<TreeItem> Tree::create_item(const std::shared_ptr<TreeItem> &parent, const std::string &text) {
    auto context = get_context();

    if (parent == nullptr) {
        root = std::make_shared<TreeItem>(context);
        root->tree = this;
        root->set_text(text);
        return root;
    }

    auto item = std::make_shared<TreeItem>(context);
    item->tree = this;
    item->parent = parent.get();
    item->set_text(text);
    parent->add_child(item);

    return item;
}

void Tree::set_item_height(float new_item_height) {
    item_height = new_item_height;
}

float Tree::get_item_height() {
    return item_height;
}

void Tree::calc_minimum_size() {
    Vec2F minimum_size;
    uint32_t uncollapsed_item_count = 0;
    root->propagate_calc_minimum_size(folding_width, 0, uncollapsed_item_count, minimum_size);

    calculated_minimum_size = {minimum_size.x, item_height * uncollapsed_item_count};
}

TreeItem::TreeItem(GuiContext* context) {
    label = std::make_shared<Label>();
    label->container_sizing.flag_v = ContainerSizingFlag::Fill;
    label->set_vertical_alignment(Alignment::Center);

    icon = std::make_shared<TextureRect>();
    icon->set_custom_minimum_size({24, 24});
    icon->set_stretch_mode(TextureRect::StretchMode::KeepAspectCentered);

    if (context) {
        collapsed_tex = std::make_shared<VectorImage>(context, get_asset_dir("icons/ArrowRight.svg"));
        expanded_tex = std::make_shared<VectorImage>(context, get_asset_dir("icons/ArrowDown.svg"));
    }

    collapse_button = std::make_shared<Button>();
    collapse_button->set_icon_normal(expanded_tex);
    collapse_button->set_text("");
    collapse_button->set_icon_expand(true);
    collapse_button->set_custom_minimum_size({24, 24});
    collapse_button->set_flat(true);

    auto callback = [this] {
        collapsed = !collapsed;
        if (collapsed) {
            collapse_button->set_icon_normal(collapsed_tex);
        } else {
            collapse_button->set_icon_normal(expanded_tex);
        }

        if (tree) {
            tree->queue_relayout();
        }
    };
    collapse_button->connect_signal("triggered", callback);

    container = std::make_shared<HBoxContainer>();
    container->set_separation(0);
    container->add_child(collapse_button);
    container->add_child(icon);
    container->add_child(label);

    collapse_button->container_sizing.flag_v = ContainerSizingFlag::Fill;

    theme_selected.bg_color = ColorU(100, 100, 100, 150);
}

uint32_t TreeItem::add_child(const std::shared_ptr<TreeItem> &item) {
    children.push_back(item);

    if (tree) {
        tree->queue_relayout();
    }

    return children.size() - 1;
}

std::shared_ptr<TreeItem> TreeItem::get_child(uint32_t idx) {
    if (idx < children.size()) {
        return children[idx];
    }
    Logger::error("Invalid child index!", "vecgui");
    return nullptr;
}

uint32_t TreeItem::get_child_count() const {
    return children.size();
}

std::vector<std::shared_ptr<TreeItem>> TreeItem::get_children() {
    return children;
}

TreeItem *TreeItem::get_parent() {
    return parent;
}

void TreeItem::propagate_input(InputEvent &event, Vec2F global_position) {
    if (!children.empty()) {
        collapse_button->input(event);
    }

    if (!collapsed) {
        auto it = children.rbegin();
        while (it != children.rend()) {
            (*it)->propagate_input(event, global_position);
            it++;
        }
    }

    input(event, global_position);
}

void TreeItem::propagate_draw_(float folding_width, uint32_t depth, float &offset_y, Vec2F global_position) {
    auto context = tree ? tree->get_context() : nullptr;
    if (!context) {
        return;
    }

    auto vector_server = context->vector_server;

    float offset_x = (float)depth * folding_width;

    // Firstly, the item height will be decided by the minimum height of the icon and label.
    float item_height = std::max(label->get_effective_minimum_size().y, icon->get_custom_minimum_size().y);

    // Then the value set by the tree is considered.
    item_height = std::max(tree->get_item_height(), item_height);

    position = {offset_x, offset_y};

    if (tree->selected_item == this) {
        vector_server->draw_style_box(
            theme_selected, Vec2F(0, offset_y) + global_position, {tree->get_size().x, item_height});
    }

    if (children.empty()) {
        // We should make the button invisible by changing the alpha value instead of the visibility.
        // Otherwise, the container layout will change and the intent will be gone.
        collapse_button->modulate = ColorU::transparent_black();

        collapse_button->set_icon_normal(nullptr);
    } else {
        collapse_button->modulate = ColorU::white();
    }

    container->set_position(Vec2F(offset_x, offset_y) + global_position);
    container->set_size({tree->get_size().x - offset_x, item_height});

    // Ensure all internal nodes have their minimum sizes calculated before layout.
    std::vector<Node*> descendants;
    dfs_postorder_ltr_traversal(container.get(), descendants);
    for (auto& node : descendants) {
        if (node->is_ui_node()) {
            dynamic_cast<NodeUi*>(node)->calc_minimum_size();
        }
    }

    layout_system(container.get());

    transform_system(container.get());

    std::vector<Node *> nodes;
    dfs_preorder_ltr_traversal(container.get(), nodes);
    for (auto &node : nodes) {
        node->update(0);
        if (node->is_ui_node()) {
            dynamic_cast<NodeUi *>(node)->clear_layout_dirty();
        }
    }

    propagate_draw(container.get());

    offset_y += item_height;

    if (!collapsed) {
        for (auto &child : children) {
            child->propagate_draw_(folding_width, depth + 1, offset_y, global_position);
        }
    }
}

void TreeItem::propagate_calc_minimum_size(float folding_width,
                                           uint32_t depth,
                                           uint32_t &uncollapsed_item_count,
                                           Vec2F &minimum_size) {
    float offset_x = (float)depth * folding_width;

    uncollapsed_item_count++;

    calc_minimum_size(container.get());

    // Firstly, the item height will be decided by the minimum height of the icon and label.
    float item_height = std::max(label->get_effective_minimum_size().y, icon->get_custom_minimum_size().y);

    // Then the value set by the tree is considered.
    item_height = std::max(tree->get_item_height(), item_height);

    float item_width = label->get_effective_minimum_size().x + icon->get_custom_minimum_size().x;
    minimum_size = minimum_size.max({item_width + offset_x, item_height});

    if (!collapsed) {
        auto it = children.rbegin();
        while (it != children.rend()) {
            (*it)->propagate_calc_minimum_size(folding_width, depth + 1, uncollapsed_item_count, minimum_size);
            it++;
        }
    }
}

void TreeItem::input(InputEvent &event, Vec2F global_position) {
    float item_height = container->get_effective_minimum_size().y;
    auto item_global_rect = (RectF(0, position.y, tree->get_size().x, position.y + item_height) + global_position);

    if (event.type == InputEventType::MouseButton) {
        auto button_event = event.args.mouse_button;

        if (!event.consumed && button_event.pressed) {
            if (item_global_rect.contains_point(button_event.position)) {
                selected = true;
                tree->selected_item = this;
                Logger::verbose("Item selected: " + label->get_text(), "vecgui");
            }
        }
    }
}

void TreeItem::set_text(const std::string &text) {
    label->set_text(text);
    if (tree) {
        tree->queue_relayout();
    }
}

void TreeItem::set_icon(const std::shared_ptr<Image> &texture) {
    icon->set_texture(texture);
    if (tree) {
        tree->queue_relayout();
    }
}

} // namespace vecgui
