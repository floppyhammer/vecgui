#include "text_edit.h"

#include <string>

#include "../../common/context.h"
#include "../../common/utils.h"
#include "../../resources/default_resource.h"
#include "../../servers/input_server.h"
#include "../../servers/vector_server.h"
#include "container/margin_container.h"

namespace vecgui {

std::string keep_numbers(const std::string &src) {
    std::string dst;

    for (const char c : src) {
        if (std::isdigit(c)) {
            dst += c;
        }
    }

    return dst;
}

TextEdit::TextEdit() {
    type = NodeType::TextEdit;

    label = std::make_shared<Label>();
    label->set_horizontal_alignment(Alignment::Begin);
    label->set_vertical_alignment(Alignment::Center);
    label->set_mouse_filter(MouseFilter::Ignore);
    label->set_word_wrap(true);

    margin_container = std::make_shared<MarginContainer>();
    margin_container->set_margin_all(4);
    margin_container->add_child(label);
    margin_container->set_mouse_filter(MouseFilter::Ignore);
    margin_container->set_anchor_flag(AnchorFlag::FullRect);

    add_embedded_child(margin_container);

    set_text("Enter text");

    callbacks_cursor_entered.emplace_back([this] {
        if (auto context = get_context()) {
            context->input_server->set_cursor(context->render_context, get_window_index(), CursorShape::IBeam);
        }
    });

    callbacks_cursor_exited.emplace_back([this] {
        if (auto context = get_context()) {
            context->input_server->set_cursor(context->render_context, get_window_index(), CursorShape::Arrow);
        }
    });
}

void TextEdit::set_text(std::string new_text) const {
    if (numbers_only) {
        new_text = keep_numbers(new_text);
    }

    label->set_text(new_text);
}

std::string TextEdit::get_text() const {
    return label->get_text();
}

void TextEdit::input(InputEvent &event) {
    auto context = get_context();
    if (!context) {
        return;
    }

    auto input_server = context->input_server;

    // Handle mouse input propagation.
    bool consume_flag = false;

    int32_t codepoint_count = label->get_text_u32().size();

    auto global_position = get_global_position();

    auto active_rect = RectF(global_position, global_position + size);

    switch (event.type) {
        case InputEventType::MouseButton: {
            auto args = event.args.mouse_button;

            if (!event.consumed && active_rect.contains_point(args.position)) {
                if (args.pressed) {
                    // Decide which codepoint the caret is at.
                    auto local_mouse_pos_to_label = args.position - label->get_global_position();
                    current_caret_index = calculate_caret_index(local_mouse_pos_to_label);
                    selection_start_index = current_caret_index;

                    is_pressed_inside = true;
                } else {
                    is_pressed_inside = false;
                }
                consume_flag = true;
            }

            if (!args.pressed) {
                is_pressed_inside = false;
            }
        } break;
        case InputEventType::MouseMotion: {
            auto args = event.args.mouse_motion;

            if (active_rect.contains_point(args.position)) {
                consume_flag = true;
            }

            if (is_pressed_inside) {
                current_caret_index = calculate_caret_index(get_local_mouse_position());
                caret_blink_timer = 0;

                Logger::verbose("Caret position: current " + std::to_string(current_caret_index) + ", selected " +
                                    std::to_string(selection_start_index),
                                "vecgui");
            }
        } break;
        case InputEventType::Text: {
            if (!focused) {
                break;
            }

            if (editable) {
                if (selection_start_index != current_caret_index) {
                    delete_selection();
                }

                if (numbers_only) {
                    if (event.args.text.codepoint < 48 || event.args.text.codepoint >= 57) {
                        break;
                    }
                }

                label->insert_text(current_caret_index, cpp11_codepoint_to_utf8(event.args.text.codepoint));

                current_caret_index++;
                selection_start_index = current_caret_index;
                caret_blink_timer = 0;
            }

            consume_flag = true;
        } break;
        case InputEventType::Key: {
            auto key_args = event.args.key;

            if (key_args.key == KeyCode::Backspace) {
                if (key_args.pressed || key_args.repeated) {
                    if (selection_start_index != current_caret_index) {
                        delete_selection();
                    } else {
                        if (editable) {
                            if (current_caret_index > 0) {
                                label->remove_text(current_caret_index - 1, 1);

                                current_caret_index--;
                                selection_start_index--;
                            }
                        }
                    }
                    caret_blink_timer = 0;
                }
            }

            if (key_args.key == KeyCode::Delete) {
                if (key_args.pressed || key_args.repeated) {
                    if (selection_start_index != current_caret_index) {
                        delete_selection();
                    } else {
                        if (editable) {
                            if (current_caret_index < codepoint_count) {
                                label->remove_text(current_caret_index, 1);
                            }
                        }
                    }
                    caret_blink_timer = 0;
                }
            }

            if (key_args.pressed || key_args.repeated) {
                if (key_args.key == KeyCode::Left) {
                    if (current_caret_index != selection_start_index) {
                        current_caret_index = std::min(selection_start_index, current_caret_index);
                        selection_start_index = current_caret_index;
                    } else if (current_caret_index > 0) {
                        current_caret_index--;
                        selection_start_index--;
                    }

                    caret_blink_timer = 0;
                } else if (key_args.key == KeyCode::Right) {
                    if (current_caret_index != selection_start_index) {
                        current_caret_index = std::max(selection_start_index, current_caret_index);
                        selection_start_index = current_caret_index;
                    } else if (current_caret_index < codepoint_count) {
                        current_caret_index++;
                        selection_start_index++;
                    }

                    caret_blink_timer = 0;
                }

                if (key_args.key == KeyCode::C && input_server->is_key_pressed(KeyCode::LeftControl)) {
                    auto start_index = std::min(selection_start_index, current_caret_index);
                    auto count = std::abs((int)selection_start_index - (int)current_caret_index);
                    std::string selected_text = label->get_sub_text(start_index, count);
                    input_server->set_clipboard(selected_text.c_str());
                }

                if (key_args.key == KeyCode::V && input_server->is_key_pressed(KeyCode::LeftControl)) {
                    if (selection_start_index != current_caret_index) {
                        delete_selection();
                    }
                    auto clipboard_text = input_server->get_clipboard();

                    if (numbers_only) {
                        clipboard_text = keep_numbers(clipboard_text);
                    }

                    std::u32string clipboard_text_u32;
                    utf8_to_utf16(clipboard_text, clipboard_text_u32);
                    label->insert_text(current_caret_index, clipboard_text);
                    current_caret_index += clipboard_text_u32.size();
                    selection_start_index = current_caret_index;
                }

                if (key_args.key == KeyCode::X && input_server->is_key_pressed(KeyCode::LeftControl)) {
                    auto start_index = std::min(selection_start_index, current_caret_index);
                    auto count = std::abs((int)selection_start_index - (int)current_caret_index);
                    std::string selected_text = label->get_sub_text(start_index, count);
                    input_server->set_clipboard(selected_text.c_str());
                    delete_selection();
                }
            }
        } break;
        default:
            break;
    }

    NodeUi::input(event);

    if (consume_flag) {
        event.consumed = true;
    }
}

void TextEdit::update(double dt) {
    NodeUi::update(dt);

    caret_blink_timer += dt;
}

void TextEdit::draw() {
    auto context = get_context();
    if (!context) {
        return;
    }

    auto vector_server = context->vector_server;

    auto global_position = get_global_position();

    auto default_theme = context->default_resource->get_default_theme();

    auto theme_normal = theme_override_normal.value_or(default_theme->text_edit.styles["normal"]);
    auto theme_focused = theme_override_focused.value_or(default_theme->text_edit.styles["focused"]);
    auto theme_caret = theme_override_caret.value_or(default_theme->text_edit.caret);
    auto theme_selection = theme_override_selection.value_or(default_theme->text_edit.styles["selection"]);

    // Draw bg.
    {
        std::optional<StyleBox> active_style_box;
        active_style_box = focused ? theme_focused : theme_normal;

        if (active_style_box.has_value()) {
            vector_server->draw_style_box(active_style_box.value(), global_position, size);
        }
    }

    // Draw selection box.
    if (focused) {
        if (selection_start_index != current_caret_index) {
            auto start = calculate_caret_position(std::min(current_caret_index, selection_start_index));
            auto end = calculate_caret_position(std::max(current_caret_index, selection_start_index));
            auto box_position = label->get_global_position() + start;
            auto box_size = Vec2F(0, (float)label->get_font_size()) + (end - start);
            vector_server->draw_style_box(theme_selection, box_position, box_size);
        }
    }

    // Draw text.
    label->draw();

    // Draw blinking caret.
    if (focused && editable) {
        theme_caret.color.a_ = 255.0f * std::ceil(std::sin(caret_blink_timer * 5.0f));

        float current_codepoint_right_edge = 0;
        if (current_caret_index > 0) {
            current_codepoint_right_edge = label->get_codepoint_right_edge_position(current_caret_index - 1);
        } else {
            current_codepoint_right_edge = 0;
        }

        auto start = label->get_global_position() + Vec2F(current_codepoint_right_edge, 3);
        auto end = start + Vec2F(0, label->get_font_size() - 6);
        vector_server->draw_style_line(theme_caret, start, end);
    }

    NodeUi::draw();
}

void TextEdit::calc_minimum_size() {
    calculated_minimum_size = margin_container->get_effective_minimum_size();
}

uint32_t TextEdit::calculate_caret_index(Vec2F local_cursor_position_to_label) {
    uint32_t closest_codepoint_index = 0;
    float closest_distance = std::numeric_limits<float>::max();

    const auto &codepoints = label->get_text_u32();

    for (int i = 0; i < codepoints.size(); i++) {
        float codepoint_right_edge = label->get_codepoint_right_edge_position(i);

        // Cursor position to the right boundary of the codepoint.
        auto distance = abs(local_cursor_position_to_label.x - codepoint_right_edge);

        if (distance < closest_distance) {
            closest_distance = distance;
            closest_codepoint_index = i;
        }
    }

    if (codepoints.empty()) {
        return 0;
    }

    // Caret is before the first codepoint.
    if (abs(local_cursor_position_to_label.x) < closest_distance) {
        return 0;
    }

    return closest_codepoint_index + 1;
}

Vec2F TextEdit::calculate_caret_position(int32_t target_caret_index) {
    if (target_caret_index > 0) {
        return {label->get_codepoint_right_edge_position(target_caret_index - 1), 0};
    }

    return {0, 0};
}

void TextEdit::grab_focus() {
    focused = true;
}

void TextEdit::release_focus() {
    focused = false;
}

void TextEdit::set_editable(bool new_value) {
    editable = new_value;
}

void TextEdit::set_numbers_only(bool new_value) {
    numbers_only = new_value;
}

void TextEdit::delete_selection() {
    if (!editable) {
        return;
    }
    auto start_index = std::min(selection_start_index, current_caret_index);
    auto count = std::abs((int)selection_start_index - (int)current_caret_index);
    label->remove_text(start_index, count);
    current_caret_index = selection_start_index = start_index;
}

} // namespace vecgui
