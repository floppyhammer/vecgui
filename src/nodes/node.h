#pragma once

#include <memory>
#include <vector>

#include "../common/any_callable.h"
#include "../common/utils.h"
#include "../servers/engine.h"
#include "../servers/input_server.h"

namespace vecgui {

enum class NodeType {
    // General.
    Node = 0,
    RenderTarget,
    Window,

    // UI.
    NodeUi,

    Container,
    CenterContainer,
    MarginContainer,
    HBoxContainer,
    VBoxContainer,
    ScrollContainer,
    TabContainer,
    CollapseContainer,
    SplitContainer,

    Button,
    MenuButton,
    OptionButton, // todo
    CheckButton,
    RadioButton,

    Slider,

    Label,
    TextEdit,
    SpinBox,
    Panel,
    TextureRect,
    Tree,
    ProgressBar,
    PopupMenu,

    NotInstantiable,

    Max,
};

std::string get_node_type_name(NodeType type);

class SceneTree;

/// Position-independent, window-independent base node.
class Node {
    friend class SceneTree;

public:
    std::string name;

    virtual ~Node() = default;

    /// Called at first time entering the tree.
    virtual void ready();

    virtual void input(InputEvent &event);

    virtual void update(double dt);

    virtual void draw();

    /// For some special nodes (e.g. ScrollContainer).
    virtual void pre_draw_children() {
    }

    virtual void post_draw_children() {
    }

    virtual void custom_ready() {
    }

    virtual void custom_update(double dt) {
    }

    virtual void custom_input(InputEvent &event) {
    }

    virtual void custom_draw() {
    }

    virtual void add_child(const std::shared_ptr<Node> &new_child);

    virtual void add_child_at_index(const std::shared_ptr<Node> &new_child, uint32_t index);

    void add_embedded_child(const std::shared_ptr<Node> &new_child);

    NodeType get_node_type() const;

    Node *get_parent() const;

    std::vector<std::shared_ptr<Node>> get_children();
    std::vector<std::shared_ptr<Node>> get_embedded_children();
    std::vector<std::shared_ptr<Node>> get_all_children();
    std::vector<std::shared_ptr<Node>> get_all_children_reversed();

    virtual std::shared_ptr<Node> get_child(size_t index);

    void remove_child(size_t index);

    void remove_all_children();

    virtual bool is_ui_node() const;

    virtual void set_visibility(bool visible);

    void show();

    void hide();

    bool get_visibility() const;

    bool get_global_visibility() const;

    uint8_t get_window_index() const;

    virtual void when_parent_size_changed(Vec2F new_size);

    /**
     * Called when the subtree structure of this node changed.
     */
    void when_subtree_changed();

    virtual void connect_signal(const std::string &signal, const AnyCallable<void> &callback);

    SceneTree *get_tree() const;

protected:
    NodeType type = NodeType::Node;

    bool ready_ = false;

    bool visible_ = true;

    std::vector<std::shared_ptr<Node>> children;

    std::vector<std::shared_ptr<Node>> embedded_children;

    // Don't use a shared pointer as it causes circular references.
    // Also, we must initialize it to null.
    Node *parent{};

    SceneTree *tree_{};

    // Called when subtree structure changes.
    std::vector<AnyCallable<void>> subtree_changed_callbacks;
};

/// Perform a depth-first-search preorder traversal from left-to-right.
/// Usages: draw nodes back-to-front, propagate transform.
/// See: https://faculty.cs.niu.edu/~mcmahon/CS241/Notes/Data_Structures/binary_tree_traversals.html
void dfs_preorder_ltr_traversal(Node *node, std::vector<Node *> &ordered_nodes);

/// Perform a depth-first-search postorder traversal from left-to-right.
/// Usages: calculate node minimum size leaf-to-root.
/// See: https://faculty.cs.niu.edu/~mcmahon/CS241/Notes/Data_Structures/binary_tree_traversals.html
void dfs_postorder_ltr_traversal(Node *node, std::vector<Node *> &ordered_nodes);

/// Perform a postorder traversal from right-to-left.
/// Usages: handle input events front-to-back.
/// See: https://faculty.cs.niu.edu/~mcmahon/CS241/Notes/Data_Structures/binary_tree_traversals.html
void dfs_postorder_rtl_traversal(Node *node, std::vector<Node *> &ordered_nodes);

void dfs_postorder_rtl_traversal_skip_priority_node_and_invisible(Node *node, std::vector<Node *> &ordered_nodes);

} // namespace vecgui
