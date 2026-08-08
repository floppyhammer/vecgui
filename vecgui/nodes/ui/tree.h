#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include "../../common/geometry.h"
#include "../../resources/font.h"
#include "../../resources/style_box.h"
#include "../../resources/vector_image.h"
#include "button/button.h"
#include "container/box_container.h"
#include "node_ui.h"

namespace vecgui {

class Tree;

class TreeItem {
    friend class Tree;

public:
    TreeItem();

    uint32_t add_child(const std::shared_ptr<TreeItem> &item);

    std::shared_ptr<TreeItem> get_child(uint32_t idx);

    uint32_t get_child_count() const;

    std::vector<std::shared_ptr<TreeItem>> get_children();

    TreeItem *get_parent();

    void propagate_input(InputEvent &event, Vec2F global_position);

    void input(InputEvent &event, Vec2F global_position);

    void propagate_draw_(float folding_width, uint32_t depth, float &offset_y, Vec2F global_position);

    void propagate_calc_minimum_size(float folding_width, uint32_t depth, uint32_t &max_depth, Vec2F &minimum_size);

    void set_text(const std::string &text);

    void set_icon(const std::shared_ptr<Image> &image);

private:
    bool collapsed = false;
    bool selected = false;

    // Local position in the tree.
    Vec2F position;

    std::shared_ptr<Button> collapse_button;
    std::shared_ptr<VectorImage> collapsed_tex, expanded_tex;

    std::shared_ptr<TextureRect> icon;

    std::shared_ptr<Label> label;

    std::shared_ptr<HBoxContainer> container;

    std::vector<std::shared_ptr<TreeItem>> children;
    TreeItem *parent = nullptr;

    Tree *tree = nullptr;

    StyleBox theme_selected;
};

class Tree : public NodeUi {
public:
    Tree();

    void input(InputEvent &event) override;

    void update(double dt) override;

    void draw() override;

    float folding_width = 24;

    std::shared_ptr<TreeItem> create_item(const std::shared_ptr<TreeItem> &parent, const std::string &text = "item");

    TreeItem *selected_item{};

    void set_item_height(float new_item_height);

    float get_item_height();

    void calc_minimum_size() override;

private:
    float item_height = 32;

    std::shared_ptr<TreeItem> root;
    std::optional<StyleBox> theme_bg;
    std::optional<StyleBox> theme_bg_focused;
};

} // namespace vecgui
