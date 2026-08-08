#include "box_container.h"

#include <algorithm>
#include <numeric>

namespace vecgui {

void BoxContainer::adjust_layout() {
    if (children.empty()) {
        return;
    }

    std::vector<NodeUi *> expanding_children;
    std::vector<NodeUi *> ui_children = get_visible_ui_children();

    for (auto &ui_child : ui_children) {
        if (!ui_child->get_visibility()) {
            continue;
        }

        if (horizontal) {
            if (ui_child->container_sizing.expand_h()) {
                expanding_children.push_back(ui_child);
            }
        } else {
            if (ui_child->container_sizing.expand_v()) {
                expanding_children.push_back(ui_child);
            }
        }
    }

    auto effective_min_size = get_effective_minimum_size();

    float available_space_for_expanding;
    if (horizontal) {
        available_space_for_expanding = size.x - effective_min_size.x;
    } else {
        available_space_for_expanding = size.y - effective_min_size.y;
    }

    size = size.max(effective_min_size);

    uint32_t expanding_child_count = expanding_children.size();

    // --- 新的水位填充算法逻辑 ---
    float target_expanding_size = 0;
    if (expanding_child_count > 0) {
        std::vector<float> min_sizes;
        for (auto &child : expanding_children) {
            auto ms = child->get_effective_minimum_size();
            min_sizes.push_back(horizontal ? ms.x : ms.y);
        }
        // 1. 从小到大排序
        std::sort(min_sizes.begin(), min_sizes.end());

        // 2. 寻找目标水位线 (Target Size)
        // 我们寻找一个 T，使得 sum(max(min_size_i, T)) = sum(min_size_i) + available_space
        float running_sum = 0;
        bool found = false;
        for (size_t k = 0; k < min_sizes.size(); ++k) {
            running_sum += min_sizes[k];
            // 假设前 k+1 个元素被提升到 T，而后面的元素保持原样
            // (k + 1) * T + sum(min_sizes[k+1...N-1]) = sum(min_sizes[0...N-1]) + available_space
            // (k + 1) * T = sum(min_sizes[0...k]) + available_space
            float t = (running_sum + available_space_for_expanding) / (float)(k + 1);

            // 检查这个 T 是否小于或等于下一个节点的最小尺寸（如果存在）
            if (k + 1 < min_sizes.size()) {
                if (t <= min_sizes[k + 1]) {
                    target_expanding_size = t;
                    found = true;
                    break;
                }
            } else {
                // 如果已经是最后一个节点
                target_expanding_size = t;
                found = true;
            }
        }
    }
    // --- 算法结束 ---

    float pos_shift = 0;
    if (expanding_child_count == 0 && alignment == BoxContainerAlignment::End) {
        pos_shift = available_space_for_expanding;
    }

    for (auto &ui_child : ui_children) {
        auto child_min_size = ui_child->get_effective_minimum_size();
        float min_dim = horizontal ? child_min_size.x : child_min_size.y;

        float occupied_space = min_dim;

        // 如果是扩展节点，分配空闲空间
        if (expanding_child_count > 0 &&
            std::find(expanding_children.begin(), expanding_children.end(), ui_child) != expanding_children.end()) {
            // 节点的最终占用空间是其最小尺寸与水位线中的较大者
            occupied_space = std::max(min_dim, target_expanding_size);
        }

        if (horizontal) {
            float real_space = occupied_space; // 默认填充/拉伸
            float pos_x = 0;
            float pos_y = 0;
            float height = 0;

            // 处理水平方向
            switch (ui_child->container_sizing.flag_h) {
                case ContainerSizingFlag::NoExpand:
                case ContainerSizingFlag::Fill: {
                    pos_x = pos_shift;
                } break;
                case ContainerSizingFlag::ShrinkStart: {
                    real_space = min_dim;
                    pos_x = pos_shift;
                } break;
                case ContainerSizingFlag::ShrinkCenter: {
                    real_space = min_dim;
                    pos_x = pos_shift + (occupied_space - real_space) * 0.5f;
                } break;
                case ContainerSizingFlag::ShrinkEnd: {
                    real_space = min_dim;
                    pos_x = pos_shift + (occupied_space - real_space);
                } break;
            }

            // 处理垂直方向 (纵向填充逻辑保持不变)
            switch (ui_child->container_sizing.flag_v) {
                case ContainerSizingFlag::NoExpand:
                case ContainerSizingFlag::Fill: {
                    height = size.y;
                    pos_y = 0;
                } break;
                case ContainerSizingFlag::ShrinkStart: {
                    height = child_min_size.y;
                    pos_y = 0;
                } break;
                case ContainerSizingFlag::ShrinkCenter: {
                    height = child_min_size.y;
                    pos_y = (size.y - height) * 0.5f;
                } break;
                case ContainerSizingFlag::ShrinkEnd: {
                    height = child_min_size.y;
                    pos_y = size.y - height;
                } break;
            }

            ui_child->set_position({pos_x, pos_y});
            ui_child->set_size({real_space, height});
        } else {
            float real_space = occupied_space;
            float pos_x = 0;
            float pos_y = 0;
            float width = 0;

            // 处理垂直方向
            switch (ui_child->container_sizing.flag_v) {
                case ContainerSizingFlag::NoExpand:
                case ContainerSizingFlag::Fill: {
                    pos_y = pos_shift;
                } break;
                case ContainerSizingFlag::ShrinkStart: {
                    real_space = min_dim;
                    pos_y = pos_shift;
                } break;
                case ContainerSizingFlag::ShrinkCenter: {
                    real_space = min_dim;
                    pos_y = pos_shift + (occupied_space - real_space) * 0.5f;
                } break;
                case ContainerSizingFlag::ShrinkEnd: {
                    real_space = min_dim;
                    pos_y = pos_shift + (occupied_space - real_space);
                } break;
            }

            // 处理水平方向
            switch (ui_child->container_sizing.flag_h) {
                case ContainerSizingFlag::NoExpand:
                case ContainerSizingFlag::Fill: {
                    width = size.x;
                    pos_x = 0;
                } break;
                case ContainerSizingFlag::ShrinkStart: {
                    width = child_min_size.x;
                    pos_x = 0;
                } break;
                case ContainerSizingFlag::ShrinkCenter: {
                    width = child_min_size.x;
                    pos_x = (size.x - width) * 0.5f;
                } break;
                case ContainerSizingFlag::ShrinkEnd: {
                    width = child_min_size.x;
                    pos_x = size.x - width;
                } break;
            }

            ui_child->set_position({pos_x, pos_y});
            ui_child->set_size({width, real_space});
        }

        pos_shift += occupied_space + separation;
    }
}

void BoxContainer::calc_minimum_size() {
    Vec2F min_size = {0, 0};

    std::vector<NodeUi *> ui_children = get_visible_ui_children();

    // Add every child's minimum size.
    for (auto &ui_child : ui_children) {
        auto child_min_size = ui_child->get_effective_minimum_size();

        if (horizontal) {
            min_size.x += child_min_size.x;
            min_size.y = std::max(min_size.y, child_min_size.y);
        } else {
            min_size.x = std::max(min_size.x, child_min_size.x);
            min_size.y += child_min_size.y;
        }
    }

    // Take separation into account.
    if (!ui_children.empty()) {
        float total_separation_size = separation * (ui_children.size() - 1);
        if (horizontal) {
            min_size.x += total_separation_size;
        } else {
            min_size.y += total_separation_size;
        }
    }

    calculated_minimum_size = min_size;
}

void BoxContainer::set_separation(float new_separation) {
    if (separation == new_separation) {
        return;
    }

    separation = new_separation;
}

void BoxContainer::set_alignment(BoxContainerAlignment new_alignment) {
    alignment = new_alignment;
}

} // namespace vecgui
