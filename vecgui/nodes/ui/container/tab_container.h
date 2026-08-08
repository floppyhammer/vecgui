#pragma once

#include <optional>

#include "../button/button.h"
#include "container.h"

namespace vecgui {

class ScrollContainer;

class TabContainer : public Container {
public:
    TabContainer();

    void update(double dt) override;

    void adjust_layout() override;

    void calc_minimum_size() override;

    std::optional<uint32_t> get_current_tab() const;

    void set_current_tab(uint32_t index);

    void set_tab_disabled(bool disabled);

    void draw() override;

    void add_child(const std::shared_ptr<Node>& new_child) override;

    void add_child_at_index(const std::shared_ptr<Node> &new_child, uint32_t index) override;

    std::optional<StyleBox> theme_override_tab_bg;

    std::optional<StyleBox> theme_override_bg;

protected:
    void reload_tab_buttons();

    std::optional<uint32_t> current_tab;

    std::shared_ptr<HBoxContainer> button_container;
    std::shared_ptr<ScrollContainer> button_scroll_container;

    std::shared_ptr<ToggleButtonGroup> tab_button_group;

    std::vector<std::shared_ptr<Button>> tab_buttons;
};

} // namespace vecgui
