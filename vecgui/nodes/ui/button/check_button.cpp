#include "check_button.h"

#include "../../../resources/default_resource.h"

namespace vecgui {

CheckButton::CheckButton() {
    type = NodeType::CheckButton;

    toggle_mode = true;

    label->set_text("Check Button");

    icon_normal_ = std::make_shared<VectorImage>(get_asset_dir("icons/CheckBox_Unchecked.svg"), true);
    icon_pressed_ = std::make_shared<VectorImage>(get_asset_dir("icons/CheckBox_Checked.svg"), true);
}

} // namespace vecgui
