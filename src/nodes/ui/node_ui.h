#pragma once

#include <vector>

#include "../../common/geometry.h"
#include "../../resources/style_box.h"
#include "../../servers/input_server.h"
#include "../../servers/vector_server.h"
#include "../node.h"

namespace vecgui {

/// How a UI node handles mouse input propagation.
enum class MouseFilter {
    Stop,   // Use input and mark it as consumed.
    Pass,   // Use input and don't mark it as consumed.
    Ignore, // Ignore input.
};

/// Anchor takes effect only when a UI node is not a child of a container.
enum class AnchorFlag {
    None,

    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,

    CenterLeft,
    CenterRight,
    CenterTop,
    CenterBottom,
    Center,

    LeftWide,
    RightWide,
    TopWide,
    BottomWide,

    VCenterWide,
    HCenterWide,

    FullRect,

    Max,
};

/// Sizing space: the space in which a UI node can adjust its position and size.

enum class ContainerSizingFlag {
    // Not expanding the sizing space in a direction. The sizing space is the minimum size of the UI node.
    NoExpand,
    // Sizing space is expanded
    Fill,         // Occupy the full space in the sizing space.
    ShrinkStart,  // Shrink to the minimum size at the start in the sizing space.
    ShrinkCenter, // Shrink to the minimum size at the center in the sizing space.
    ShrinkEnd,    // Shrink to the minimum size at the end in the sizing space.
};

/// How a parent container organizes this UI node.
struct ContainerSizing {
    /// Control how the position & size changes in the horizontal sizing space.
    ContainerSizingFlag flag_h = ContainerSizingFlag::NoExpand;
    /// Control how the position & size changes in the vertical sizing space.
    ContainerSizingFlag flag_v = ContainerSizingFlag::NoExpand;

    bool expand_h() const {
        return flag_h != ContainerSizingFlag::NoExpand;
    }

    bool expand_v() const {
        return flag_v != ContainerSizingFlag::NoExpand;
    }
};

class NodeUi : public Node {
public:
    NodeUi();

    virtual void set_position(Vec2F new_position);

    virtual Vec2F get_position() const;

    virtual void set_size(Vec2F new_size);

    virtual Vec2F get_size() const;

    virtual void set_custom_minimum_size(Vec2F new_size);

    virtual Vec2F get_custom_minimum_size() const;

    Vec2F get_effective_minimum_size() const;

    /// Calculates the minimum size of this node, considering all its children's sizing effect.
    virtual void calc_minimum_size();

    void apply_anchor();

    virtual void adjust_layout();

    void queue_relayout();

    bool is_layout_dirty() const {
        return layout_is_dirty;
    }

    void clear_layout_dirty() {
        layout_is_dirty = false;
    }

    bool is_ui_node() const override {
        return true;
    }

    Vec2F get_global_position() const;

    Vec2F get_draw_position() const;

    void calc_global_position(Vec2F parent_global_position);

    virtual bool ignore_mouse_input_outside_rect() const {
        return false;
    }

    void draw() override;

    void set_mouse_filter(MouseFilter filter);

    void set_pivot(Vec2F new_pivot);

    Vec2F get_pivot() const;

    ContainerSizing container_sizing{};

    Vec2F get_local_mouse_position() const;

    virtual void grab_focus();

    virtual void release_focus();

    ColorU get_global_modulate();

    /**
     * Check if the node is a child of a container.
     * @return
     */
    bool is_inside_container() const;

    Vec2F get_max_child_min_size() const;

    ColorU modulate = ColorU::white();
    ColorU self_modulate = ColorU::white();

    /**
     * Adjust the node's position and size according to the anchor flag.
     */
    void set_anchor_flag(AnchorFlag new_flag);

    AnchorFlag get_anchor_flag() const;

    void when_parent_size_changed(Vec2F new_size) override;

    void when_cursor_entered();

    void when_cursor_exited();

    void connect_signal(const std::string &signal, const AnyCallable<void> &callback) override;

    std::optional<StyleBox> debug_box;

protected:
    Vec2F size{16, 16};

    /// Transform.
    Vec2F position{0};
    Vec2F scale{1};
    float rotation = 0;

    /// Transform origin. Unit: normalized.
    Vec2F pivot{0};

    Vec2F calculated_global_position{0};

    bool layout_is_dirty = true;

    bool focused = false;

    bool is_pressed_inside = false;

    Vec2F custom_minimum_size{};

    /// Minimum size with all elements considered. Differs from user set `minimum_size`.
    Vec2F calculated_minimum_size{};

    Vec2F local_mouse_position;

    std::optional<Vec2F> pressed_mouse_position;

    AnchorFlag anchor_mode = AnchorFlag::None;

    bool is_cursor_inside = false;

    void update(double dt) override;

    void input(InputEvent &input_event) override;

    void cursor_entered();

    void cursor_exited();

    float alpha = 1.0;

    MouseFilter mouse_filter = MouseFilter::Stop;

    std::vector<AnyCallable<void>> callbacks_cursor_entered;
    std::vector<AnyCallable<void>> callbacks_cursor_exited;
    std::vector<AnyCallable<void>> callbacks_focus_released;
    std::vector<AnyCallable<void>> callbacks_parent_size_changed;
};

} // namespace vecgui
