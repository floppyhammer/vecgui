#include "node.h"

#include <ranges>
#include <string>

#include "../servers/render_server.h"
#include "proxy_window.h"
#include "ui/node_ui.h"

namespace vecgui {

const char *NodeNames[] = {
    "Node",
    "Window",

    "NodeUi",

    "Container",
    "CenterContainer",
    "MarginContainer",
    "HBoxContainer",
    "VBoxContainer",
    "ScrollContainer",
    "TabContainer",

    "Button",
    "MenuButton",
    "OptionButton",
    "CheckButton",
    "RadioButton",

    "Label",
    "TextEdit",
    "SpinBox",
    "Panel",
    "TextureRect",
    "Tree",
    "ProgressBar",
    "PopupMenu",

    "NotInstantiable",

    "Max",
};

std::string get_node_type_name(NodeType type) {
    return NodeNames[(uint32_t)type];
}

void dfs_preorder_ltr_traversal(Node *node, std::vector<Node *> &ordered_nodes) {
    if (node == nullptr) {
        return;
    }

    // Debug print.
    // std::cout << "Node: " << get_node_type_name(node->type) << std::endl;

    ordered_nodes.push_back(node);

    for (auto &child : node->get_all_children()) {
        dfs_preorder_ltr_traversal(child.get(), ordered_nodes);
    }
}

void dfs_postorder_ltr_traversal(Node *node, std::vector<Node *> &ordered_nodes) {
    if (node == nullptr) {
        return;
    }

    for (auto &child : node->get_all_children()) {
        dfs_postorder_ltr_traversal(child.get(), ordered_nodes);
    }

    // Debug print.
    // std::cout << "Node: " << get_node_type_name(node->type) << std::endl;

    ordered_nodes.push_back(node);
}

void dfs_postorder_rtl_traversal(Node *node, std::vector<Node *> &ordered_nodes) {
    if (node == nullptr) {
        return;
    }

    for (auto &riter : std::ranges::reverse_view(node->get_all_children())) {
        dfs_postorder_rtl_traversal(riter.get(), ordered_nodes);
    }

    // Debug print.
    // std::cout << "Node: " << get_node_type_name(node->type) << std::endl;

    ordered_nodes.push_back(node);
}

void dfs_postorder_rtl_traversal_skip_priority_node_and_invisible(Node *node, std::vector<Node *> &ordered_nodes) {
    if (node == nullptr || !node->get_visibility()) {
        return;
    }

    // Skip RenderTarget, ProxyWindow and all its children.
    if (dynamic_cast<RenderTarget *>(node)) {
        return;
    }

    auto all_children = node->get_all_children();

    for (auto &riter : std::ranges::reverse_view(all_children)) {
        dfs_postorder_rtl_traversal_skip_priority_node_and_invisible(riter.get(), ordered_nodes);
    }

    ordered_nodes.push_back(node);
}

void Node::ready() {
    if (ready_) {
        return;
    }

    ready_ = true;

    on_ready();
}

void Node::input(InputEvent &event) {
    on_input(event);
}

void Node::update(double dt) {
    on_update(dt);
}

void Node::draw() {
    if (!visible_) {
        return;
    }

    on_draw();
}

Node *Node::get_parent() const {
    return parent;
}

std::vector<std::shared_ptr<Node>> Node::get_children() {
    return children;
}

std::vector<std::shared_ptr<Node>> Node::get_embedded_children() {
    return embedded_children;
}

std::vector<std::shared_ptr<Node>> Node::get_all_children() {
    std::vector<std::shared_ptr<Node>> all_children;
    all_children.reserve(embedded_children.size() + children.size());
    all_children.insert(all_children.end(), embedded_children.begin(), embedded_children.end());
    all_children.insert(all_children.end(), children.begin(), children.end());

    return all_children;
}

std::vector<std::shared_ptr<Node>> Node::get_all_children_reversed() {
    auto all_children = get_all_children();

    std::ranges::reverse(all_children);

    return all_children;
}

void Node::add_child(const std::shared_ptr<Node> &new_child) {
    assert(new_child && new_child.get() != this);

    if (std::find(children.begin(), children.end(), new_child) != children.end()) {
        std::cout << "Attempted to add a repeated child!" << std::endl;
        return;
    }

    // Set self as the parent of the new node.
    new_child->parent = this;
    new_child->tree_ = tree_;

    children.push_back(new_child);

    if (this->is_ui_node()) {
        dynamic_cast<NodeUi *>(this)->queue_relayout();
    }
}

void Node::add_child_at_index(const std::shared_ptr<Node> &new_child, uint32_t index) {
    assert(new_child && new_child.get() != this);

    if (std::find(children.begin(), children.end(), new_child) != children.end()) {
        std::cout << "Attempted to add a repeated child!" << std::endl;
        return;
    }

    if (index >= children.size()) {
        std::cout << "Attempted to add a child at a non-exist index!" << std::endl;
    }

    // Set self as the parent of the new node.
    new_child->parent = this;
    new_child->tree_ = tree_;

    children.insert(children.begin() + index, new_child);

    if (this->is_ui_node()) {
        dynamic_cast<NodeUi *>(this)->queue_relayout();
    }
}

void Node::add_embedded_child(const std::shared_ptr<Node> &new_child) {
    if (std::find(embedded_children.begin(), embedded_children.end(), new_child) != embedded_children.end()) {
        std::cout << "Attempted to add a repeated embedded child!" << std::endl;
        return;
    }

    // Set self as the parent of the new node.
    new_child->parent = this;
    new_child->tree_ = tree_;

    embedded_children.push_back(new_child);

    if (this->is_ui_node()) {
        dynamic_cast<NodeUi *>(this)->queue_relayout();
    }
}

std::shared_ptr<Node> Node::get_child(size_t index) {
    if (index > children.size()) {
        return nullptr;
    }

    return children[index];
}

void Node::remove_child(size_t index) {
    if (index < 0 || index >= children.size()) {
        return;
    }
    children.erase(children.begin() + index);

    if (this->is_ui_node()) {
        dynamic_cast<NodeUi *>(this)->queue_relayout();
    }
}

void Node::remove_all_children() {
    children.clear();

    if (this->is_ui_node()) {
        dynamic_cast<NodeUi *>(this)->queue_relayout();
    }
}

bool Node::is_ui_node() const {
    return false;
}

void Node::set_visibility(bool visible) {
    if (visible_ == visible) {
        return;
    }

    visible_ = visible;

    if (this->is_ui_node()) {
        dynamic_cast<NodeUi *>(this)->queue_relayout();
    }
}

void Node::show() {
    set_visibility(true);
}

void Node::hide() {
    set_visibility(false);
}

bool Node::get_visibility() const {
    return visible_;
}

bool Node::get_global_visibility() const {
    if (parent) {
        return parent->get_global_visibility() && get_visibility();
    }

    return get_visibility();
}

uint8_t Node::get_window_index() const {
    if (type == NodeType::Window) {
#ifdef VECGUI_USE_WINDOW
        auto sub_window_node = (ProxyWindow *)this;
        auto raw_window = sub_window_node->get_raw_window();
        return raw_window ? raw_window->window_index : 0;
#else
        return 0;
#endif
    }

    if (parent) {
        return parent->get_window_index();
    }

    return 0;
}

void Node::when_parent_size_changed(Vec2F new_size) {
    for (auto &child : get_all_children()) {
        child->when_parent_size_changed(new_size);
    }
}

void Node::when_subtree_changed() {
    for (auto &callback : subtree_changed_callbacks) {
        callback();
    }

    // Branch->root signal propagation.
    if (parent) {
        parent->when_subtree_changed();
    }
}

void Node::connect_signal(const std::string &signal, const AnyCallable<void> &callback) {
    if (signal == "subtree_changed") {
        subtree_changed_callbacks.push_back(callback);
    }
}

NodeType Node::get_node_type() const {
    return type;
}

SceneTree *Node::get_tree() const {
    return tree_;
}

} // namespace vecgui
