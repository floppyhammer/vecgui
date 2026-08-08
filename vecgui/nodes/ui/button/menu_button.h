#pragma once

#include "button.h"

namespace vecgui {

class PopupMenu;
class ScrollContainer;

class MenuButton : public Button {
public:
    MenuButton();

    std::weak_ptr<PopupMenu> get_popup_menu() const;

    void connect_signal(const std::string &signal, const AnyCallable<void> &callback) override;

    /// Manually select an item.
    void select_item(uint32_t item_index);

    std::optional<uint32_t> get_selected_item_index() const;

    std::string get_selected_item_text() const;

    std::string get_selected_item_meta() const;

    void calc_minimum_size() override;

protected:
    std::optional<uint32_t> selected_item_index;

    std::shared_ptr<PopupMenu> menu;

    std::vector<AnyCallable<void>> selected_callbacks;

    void when_item_selected(uint32_t item_index);
};

} // namespace vecgui
