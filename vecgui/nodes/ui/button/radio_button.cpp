#include "radio_button.h"

#include "../../../resources/resource.h"
#include "../../../resources/vector_image.h"

namespace vecgui {

RadioButton::RadioButton() {
    type = NodeType::RadioButton;

    toggle_mode = true;

    label->set_text("Radio Button");
}

void RadioButton::on_ready() {
    Button::on_ready();

    auto context = get_context();
    if (!context) {
        return;
    }

    if (!icon_normal_) {
        icon_normal_ = std::make_shared<VectorImage>(context, get_asset_dir("icons/GuiRadioUnchecked.svg"), true);
    }
    if (!icon_pressed_) {
        icon_pressed_ = std::make_shared<VectorImage>(context, get_asset_dir("icons/GuiRadioChecked.svg"), true);
    }
}

} // namespace vecgui
