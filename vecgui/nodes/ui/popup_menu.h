#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include "../../common/geometry.h"
#include "../../resources/font.h"
#include "../../resources/style_box.h"
#include "../../resources/vector_image.h"
#include "container/box_container.h"
#include "label.h"
#include "node_ui.h"
#include "texture_rect.h"

namespace vecgui {

class PopupMenu;

class ScrollContainer;

class Button;

class MarginContainer;

// TODO: we should make it flexible to change a popup menu's parent from a NodeUi to a ProxyWindow,
// so we can have intuitive windowed popup support.
class PopupMenu : public NodeUi {
public:
    PopupMenu();

    void input(InputEvent &event) override;

    void draw() override;

    void clear_items();

    void set_visibility(bool visible) override;

    void create_item(const std::string &text = "item");

    void set_item_meta(uint32_t item_index, std::string meta);

    void set_item_height(float new_item_height);

    float get_item_height() const;

    int get_item_count() const;

    std::string get_item_text(uint32_t item_index) const;

    std::string get_item_mata(uint32_t item_index) const;

    void connect_signal(const std::string &signal, const AnyCallable<void> &callback) override;

    void calc_minimum_size() override;

    void set_popup_position(Vec2F new_position, float new_button_height);

    void adjust_layout() override;

    std::optional<StyleBox> theme_override_bg;

private:
    void when_item_selected(uint32_t item_index);
    void when_popup_hide();

    Vec2F popup_position;
    float button_height;

    std::shared_ptr<ScrollContainer> scroll_container_;

    std::shared_ptr<VBoxContainer> vbox_container_;
    std::shared_ptr<MarginContainer> margin_container_;

    std::vector<std::shared_ptr<Button>> items_;

    std::vector<std::string> meta_;

    float item_height_ = 48;

    std::vector<AnyCallable<void>> item_selected_callbacks;

    std::vector<AnyCallable<void>> popup_hide_callbacks;
};

} // namespace vecgui
