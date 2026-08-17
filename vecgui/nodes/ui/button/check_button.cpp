#include "check_button.h"

#include "../../../resources/resource.h"
#include "../../../resources/vector_image.h"

namespace vecgui {

CheckButton::CheckButton() {
    type = NodeType::CheckButton;

    toggle_mode = true;

    label->set_text("Check Button");
}

void CheckButton::on_ready() {
    Button::on_ready();

    auto context = get_context();
    if (!context) {
        return;
    }

    if (!icon_normal_) {
        icon_normal_ = std::make_shared<VectorImage>(context, get_asset_dir("icons/CheckBox_Unchecked.svg"), true);
    }
    if (!icon_pressed_) {
        icon_pressed_ = std::make_shared<VectorImage>(context, get_asset_dir("icons/CheckBox_Checked.svg"), true);
    }
}

} // namespace vecgui
