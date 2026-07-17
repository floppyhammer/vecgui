#include "scene_tree.h"

#include <algorithm>
#include <execution>
#include <future>

#include "../servers/render_server.h"
#include "proxy_window.h"

namespace vecgui {

SceneTree::SceneTree(Vec2I primary_window_size) {
    auto primary_window = std::make_shared<ProxyWindow>(primary_window_size, 0);
    primary_window->name = "Primary window";

    root = primary_window;
    root->tree_ = this;
}

SceneTree::SceneTree() {
    root = std::make_shared<ProxyWindow>(Vec2I{1, 1}); // Create a headless proxy window.
    root->name = "Root";
    root->tree_ = this;
}

void propagate_input(Node* node, InputEvent& event) {
    if (!node->get_visibility()) {
        return;
    }

    for (auto& child : node->get_all_children_reversed()) {
        if (typeid(*child) == typeid(ProxyWindow) || !node->get_visibility()) {
            continue;
        }

        // Do not propagate out-of-bounds mouse input events if they are explicitly ignored.
        if (node->is_ui_node()) {
            auto ui_node = dynamic_cast<NodeUi*>(node);

            if (ui_node->get_node_type() == NodeType::ScrollContainer) {
                // Intercept out-of-scope mouse input events.
                auto global_position = ui_node->get_global_position();

                auto active_rect = RectF(global_position, global_position + ui_node->get_size());

                switch (event.type) {
                    case InputEventType::MouseMotion:
                    case InputEventType::MouseButton:
                    case InputEventType::MouseScroll: {
                        if (!active_rect.contains_point(InputServer::get_singleton()->cursor_position)) {
                            // todo: send an unfocus signal
                            InputEvent dummy_event = event; // Copy
                            dummy_event.consumed = false;
                            dummy_event.type = InputEventType::MouseMotion;
                            dummy_event.args.mouse_motion.position = {-99999, -99999};
                            propagate_input(child.get(), dummy_event);
                            continue;
                        }
                    } break;
                    default:
                        break;
                }
            }
        }

        propagate_input(child.get(), event);
    }

    node->input(event);
}

void input_system(Node* root, std::vector<InputEvent>& input_queue) {
    std::vector<Node*> priority_nodes;
    {
        std::vector<Node*> nodes;
        dfs_postorder_rtl_traversal(root, nodes);

        for (auto& node : nodes) {
            if (typeid(*node) == typeid(ProxyWindow) || typeid(*node) == typeid(PopupMenu)) {
                priority_nodes.push_back(node);
            }
        }
    }

    for (auto& p_node : priority_nodes) {
        if (!p_node->get_visibility()) {
            continue;
        }

        for (auto& event : input_queue) {
            if (event.window_index != p_node->get_window_index()) {
                continue;
            }

            propagate_input(p_node, event);
        }
    }
}

void propagate_transform(NodeUi* node, Vec2F parent_global_transform) {
    if (node == nullptr) {
        return;
    }

    node->calc_global_position(parent_global_transform);

    for (auto& child : node->get_all_children()) {
        if (child->is_ui_node()) {
            auto ui_child = dynamic_cast<NodeUi*>(child.get());
            propagate_transform(ui_child, node->get_global_position());
        }
    }
}

void transform_system(Node* root) {
    if (root == nullptr) {
        return;
    }

    // Collect all orphan UI nodes.
    std::vector<Node*> nodes;
    std::vector<NodeUi*> orphan_ui_nodes;
    dfs_preorder_ltr_traversal(root, nodes);
    for (auto& node : nodes) {
        if (node->is_ui_node()) {
            // Has no parent or no UI parent.
            if (node->get_parent() == nullptr || !node->get_parent()->is_ui_node()) {
                auto ui_node = dynamic_cast<NodeUi*>(node);
                orphan_ui_nodes.push_back(ui_node);
            }
        }
    }

// There's no transform dependency between orphan UI nodes.
#if defined(__APPLE__) || defined(__ANDROID__)
    std::ranges::for_each(orphan_ui_nodes, [](NodeUi* ui_node) { propagate_transform(ui_node, Vec2F{}); });
#else
    std::for_each(std::execution::par, orphan_ui_nodes.begin(), orphan_ui_nodes.end(), [](NodeUi* ui_node) {
        propagate_transform(ui_node, Vec2F{});
    });
#endif
}

void propagate_draw(Node* node) {
    node->draw();

    node->pre_draw_children();

    for (auto& child : node->get_all_children()) {
        if (!node->get_visibility()) {
            continue;
        }
        // Don't propagate to ProxyWindows/PopupMenus as we'll handle them differently.
        if (typeid(*child) == typeid(ProxyWindow)) {
            continue;
        }
        if (typeid(*child) == typeid(PopupMenu)) {
            continue;
        }

        propagate_draw(child.get());
    }

    node->post_draw_children();
}

void calc_minimum_size(Node* root) {
    std::vector<Node*> descendants;
    dfs_postorder_ltr_traversal(root, descendants);
    for (auto& node : descendants) {
        if (node->is_ui_node()) {
            auto ui_node = dynamic_cast<NodeUi*>(node);
            if (ui_node->is_layout_dirty()) {
                ui_node->calc_minimum_size();
            }
        }
    }
}

void layout_system(Node* root) {
    std::vector<Node*> nodes;
    dfs_preorder_ltr_traversal(root, nodes);
    for (auto& node : nodes) {
        if (node->is_ui_node()) {
            auto ui_node = dynamic_cast<NodeUi*>(node);
            if (ui_node->is_layout_dirty()) {
                ui_node->apply_anchor();
                ui_node->adjust_layout();
                ui_node->clear_layout_dirty();
            }
        }
    }
}

void SceneTree::process(double dt) {
    if (root == nullptr) {
        return;
    }

    auto primary_window = get_primary_window();
    if (primary_window && primary_window->get_resize_flag()) {
        Logger::info("Notify window resizing", "vecgui");
        notify_primary_window_size_changed(primary_window->get_logical_size());
    }

    // OpenGL calls in input callbacks cannot be made from another thread.
    input_system(root.get(), InputServer::get_singleton()->input_queue);

    // Get ready from-back-to-front.
    std::vector<Node*> nodes;
    dfs_preorder_ltr_traversal(root.get(), nodes);
    for (auto& node : nodes) {
        node->ready();
    }

    // Update from-back-to-front.
    for (auto& node : nodes) {
        if (!node->ready_) {
            continue;
        }
        node->update(dt);
    }

    // Run calc_minimum_size() depth-first.
    calc_minimum_size(root.get());

    // Adjust container layouts.
    layout_system(root.get());

    // Update global transform for each node.
    transform_system(root.get());
}

bool SceneTree::render() const {
    // Collect all windows.
    std::vector<ProxyWindow*> sub_windows;
    {
        std::vector<Node*> nodes;
        dfs_preorder_ltr_traversal(root.get(), nodes);
        for (auto& node : nodes) {
            if (typeid(*node) == typeid(ProxyWindow)) {
                auto sub_window = dynamic_cast<ProxyWindow*>(node);
                sub_windows.push_back(sub_window);
            }
        }
    }

    // Draw sub-windows.
    for (const auto& w : sub_windows) {
        if (!w->get_visibility()) {
            continue;
        }

        // Skip headless windows during standard render loop,
        // they are expected to be handled by OffscreenApp manually.
        if (w->get_raw_window() == nullptr) {
            continue;
        }

        // Get all pop menus that belong to this window.
        std::vector<PopupMenu*> popup_menus;
        {
            std::vector<Node*> nodes;
            dfs_preorder_ltr_traversal(w, nodes);
            for (auto& node : nodes) {
                if (typeid(*node) == typeid(PopupMenu)) {
                    auto popup_menu = dynamic_cast<PopupMenu*>(node);
                    popup_menus.push_back(popup_menu);
                }
            }
        }

        w->pre_draw_propagation();

        // Collect renderable objects
        propagate_draw(w);

        // Draw popup menus
        for (const auto& m : popup_menus) {
            if (!m->get_visibility()) {
                continue;
            }

            propagate_draw(m);
        }

        // Submit render commands
        w->post_draw_propagation();
    }

    auto primary_window = get_primary_window();
    bool should_close = primary_window ? primary_window->should_close() : false;

    return should_close || quited;
}

void SceneTree::notify_primary_window_size_changed(Vec2I new_size) const {
    root->when_parent_size_changed(new_size.to_f32());
}

std::shared_ptr<Node> SceneTree::get_root() const {
    return std::static_pointer_cast<Node>(root);
}

void SceneTree::quit() {
    quited = true;
}

std::shared_ptr<Pathfinder::Window> SceneTree::get_primary_window() const {
    return root->get_raw_window();
}

} // namespace vecgui
