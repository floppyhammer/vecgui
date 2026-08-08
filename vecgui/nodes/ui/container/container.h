#pragma once

#include "../node_ui.h"

namespace vecgui {

/// A Container adjusts layouts for its UI children automatically.
class Container : public NodeUi {
public:
    Container();

    void update(double dt) override;

    void calc_minimum_size() override;

    void draw() override;

    std::optional<StyleBox> theme_override_bg;

    /// The most important method for containers.
    /// This adjusts its own size (not position), adjusts its children's sizes and local positions.
    void adjust_layout() override;

protected:
    std::vector<NodeUi *> get_visible_ui_children() const;
};

} // namespace vecgui
