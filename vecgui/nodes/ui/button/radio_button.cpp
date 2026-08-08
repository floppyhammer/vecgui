#include "radio_button.h"

#include "../../../resources/default_resource.h"

namespace vecgui {

RadioButton::RadioButton() {
    type = NodeType::RadioButton;

    toggle_mode = true;

    label->set_text("Radio Button");

    icon_normal_ = std::make_shared<VectorImage>(get_asset_dir("icons/GuiRadioUnchecked.svg"), true);
    icon_pressed_ = std::make_shared<VectorImage>(get_asset_dir("icons/GuiRadioChecked.svg"), true);
}

} // namespace vecgui
