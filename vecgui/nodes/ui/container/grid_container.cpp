#include "grid_container.h"
#include <algorithm>
#include <vector>

namespace vecgui {

void GridContainer::adjust_layout() {
    if (children.empty() || col_limit == 0) {
        return;
    }

    std::vector<NodeUi *> ui_children = get_visible_ui_children();
    if (ui_children.empty()) {
        return;
    }

    uint32_t col_num = std::min((uint32_t)ui_children.size(), col_limit);
    if (col_num == 0) col_num = 1;
    uint32_t row_num = (ui_children.size() + col_num - 1) / col_num;

    std::vector<float> col_widths(col_num, min_col_width);
    std::vector<float> row_heights(row_num, min_row_height);

    // 1. Calculate max width for each column and max height for each row
    for (int child_idx = 0; child_idx < ui_children.size(); child_idx++) {
        auto &child = ui_children[child_idx];
        if (!child->get_visibility() || !child->is_ui_node()) {
            continue;
        }

        int row_idx = child_idx / col_num;
        int col_idx = child_idx % col_num;

        auto child_min_size = child->get_effective_minimum_size();
        col_widths[col_idx] = std::max(col_widths[col_idx], child_min_size.x);
        row_heights[row_idx] = std::max(row_heights[row_idx], child_min_size.y);
    }

    // 2. Position and size each child within its cell based on container_sizing
    for (int child_idx = 0; child_idx < ui_children.size(); child_idx++) {
        auto &child = ui_children[child_idx];
        if (!child->get_visibility() || !child->is_ui_node()) {
            continue;
        }

        int row_idx = child_idx / col_num;
        int col_idx = child_idx % col_num;

        float cell_x = 0;
        for (int c = 0; c < col_idx; c++) {
            cell_x += col_widths[c] + separation;
        }

        float cell_y = 0;
        for (int r = 0; r < row_idx; r++) {
            cell_y += row_heights[r] + separation;
        }

        float cell_w = col_widths[col_idx];
        float cell_h = row_heights[row_idx];

        auto child_min_size = child->get_effective_minimum_size();

        float final_w = child_min_size.x;
        float final_h = child_min_size.y;
        float pos_x = cell_x;
        float pos_y = cell_y;

        // Horizontal sizing
        switch (child->container_sizing.flag_h) {
            case ContainerSizingFlag::NoExpand:
            case ContainerSizingFlag::Fill:
                final_w = cell_w;
                pos_x = cell_x;
                break;
            case ContainerSizingFlag::ShrinkStart:
                final_w = child_min_size.x;
                pos_x = cell_x;
                break;
            case ContainerSizingFlag::ShrinkCenter:
                final_w = child_min_size.x;
                pos_x = cell_x + (cell_w - child_min_size.x) * 0.5f;
                break;
            case ContainerSizingFlag::ShrinkEnd:
                final_w = child_min_size.x;
                pos_x = cell_x + cell_w - child_min_size.x;
                break;
        }

        // Vertical sizing
        switch (child->container_sizing.flag_v) {
            case ContainerSizingFlag::NoExpand:
            case ContainerSizingFlag::Fill:
                final_h = cell_h;
                pos_y = cell_y;
                break;
            case ContainerSizingFlag::ShrinkStart:
                final_h = child_min_size.y;
                pos_y = cell_y;
                break;
            case ContainerSizingFlag::ShrinkCenter:
                final_h = child_min_size.y;
                pos_y = cell_y + (cell_h - child_min_size.y) * 0.5f;
                break;
            case ContainerSizingFlag::ShrinkEnd:
                final_h = child_min_size.y;
                pos_y = cell_y + cell_h - child_min_size.y;
                break;
        }

        child->set_position({pos_x, pos_y});
        child->set_size({final_w, final_h});
    }

    // Set container self size
    float total_w = 0;
    for (float w : col_widths) total_w += w;
    if (col_num > 1) total_w += separation * (col_num - 1);

    float total_h = 0;
    for (float h : row_heights) total_h += h;
    if (row_num > 1) total_h += separation * (row_num - 1);

    set_size({total_w, total_h});
}

void GridContainer::calc_minimum_size() {
    std::vector<NodeUi *> ui_children = get_visible_ui_children();
    if (ui_children.empty() || col_limit == 0) {
        calculated_minimum_size = {0, 0};
        min_col_width = 0;
        min_row_height = 0;
        return;
    }

    uint32_t col_num = std::min((uint32_t)ui_children.size(), col_limit);
    if (col_num == 0) col_num = 1;
    uint32_t row_num = (ui_children.size() + col_num - 1) / col_num;

    std::vector<float> col_widths(col_num, min_col_width);
    std::vector<float> row_heights(row_num, min_row_height);

    for (int child_idx = 0; child_idx < ui_children.size(); child_idx++) {
        auto ui_child = ui_children[child_idx];
        if (!ui_child->get_visibility() || !ui_child->is_ui_node()) {
            continue;
        }

        int row_idx = child_idx / col_num;
        int col_idx = child_idx % col_num;

        auto child_min_size = ui_child->get_effective_minimum_size();
        col_widths[col_idx] = std::max(col_widths[col_idx], child_min_size.x);
        row_heights[row_idx] = std::max(row_heights[row_idx], child_min_size.y);
    }

    float total_w = 0;
    for (float w : col_widths) total_w += w;
    if (col_num > 1) total_w += separation * (col_num - 1);

    float total_h = 0;
    for (float h : row_heights) total_h += h;
    if (row_num > 1) total_h += separation * (row_num - 1);

    calculated_minimum_size = {total_w, total_h};
}

void GridContainer::set_separation(float new_separation) {
    if (separation == new_separation) {
        return;
    }
    separation = new_separation;
}

void GridContainer::set_column_limit(uint32_t new_limit) {
    col_limit = new_limit;
}

void GridContainer::set_item_shrinking(bool new_shrinking) {
    shrinking = new_shrinking;
}

} // namespace vecgui
