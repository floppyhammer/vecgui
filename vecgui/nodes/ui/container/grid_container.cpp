#include "grid_container.h"

namespace vecgui {

void GridContainer::adjust_layout() {
    if (children.empty()) {
        return;
    }

    std::vector<NodeUi *> ui_children = get_visible_ui_children();

    int row_num = ui_children.size() / col_limit;
    int col_num = std::min((uint32_t)ui_children.size(), col_limit);

    float average_col_width = (size.x - separation * static_cast<float>(col_num - 1)) / static_cast<float>(col_num);
    float average_row_height = (size.y - separation * static_cast<float>(row_num - 1)) / static_cast<float>(row_num);

    float actual_col_width = std::max(average_col_width, min_col_width);
    float actual_row_height = std::max(average_row_height, min_row_height);

    // In the first loop, we need do some calculation.
    for (int child_idx = 0; child_idx < ui_children.size(); child_idx++) {
        auto &child = ui_children[child_idx];

        // We only care about visible GUI nodes in a container.
        if (!child->get_visibility() || !child->is_ui_node()) {
            continue;
        }

        int row_idx = child_idx / col_num;
        int col_idx = child_idx % col_num;

        float pos_x = col_idx * separation;
        for (int col = 0; col < col_idx; col++) {
            pos_x += actual_col_width;
        }
        float pos_y = row_idx * separation;
        for (int row = 0; row < row_idx; row++) {
            pos_y += actual_row_height;
        }

        if (shrinking) {
            // Add position offset if the child shrinks.
            auto child_min_size = child->get_effective_minimum_size();
            pos_x += (actual_col_width - child_min_size.x) * 0.5;
            pos_y += (actual_row_height - child_min_size.y) * 0.5;

            child->set_position({pos_x, pos_y});
            child->set_size({child_min_size.x, child_min_size.y});
        } else {
            child->set_position({pos_x, pos_y});
            child->set_size({actual_col_width, actual_row_height});
        }
    }

    // Set self size.
    set_size({actual_col_width * col_num + separation * (col_num - 1),
              actual_row_height * row_num + separation * (row_num - 1)});
}

void GridContainer::calc_minimum_size() {
    std::vector<NodeUi *> ui_children = get_visible_ui_children();

    int row_num = ui_children.size() / col_limit;
    int col_num = std::min((uint32_t)ui_children.size(), col_limit);

    min_col_width = 0;
    min_row_height = 0;

    for (int child_idx = 0; child_idx < ui_children.size(); child_idx++) {
        auto ui_child = ui_children[child_idx];

        auto child_min_size = ui_child->get_effective_minimum_size();

        min_col_width = std::max(min_col_width, child_min_size.x);
        min_row_height = std::max(min_row_height, child_min_size.y);
    }

    calculated_minimum_size = {min_col_width * col_num, min_row_height * row_num};
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
