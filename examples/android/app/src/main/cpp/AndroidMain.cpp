#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>

#include <iostream>
#include <random>

#include "vecgui/app.h"

vecgui::App *app;

constexpr bool USE_VULKAN = false;

constexpr int DPI_STANDARD = 96;

using namespace vecgui;

using Pathfinder::Vec2;
using Pathfinder::Vec3;

class MyNode : public Node {
    std::shared_ptr<ToggleButtonGroup> button_group;

    void on_ready() override {
        auto vbox_container = std::make_shared<VBoxContainer>();
        vbox_container->set_separation(8);
        vbox_container->set_position({10, 10});
        add_child(vbox_container);

        {
            auto button = std::make_shared<Button>();
            button->container_sizing.flag_h = ContainerSizingFlag::ShrinkStart;
            vbox_container->add_child(button);

            auto callback = []() { Logger::info("Button triggered"); };
            button->connect_signal("triggered", callback);
        }

        {
            auto button = std::make_shared<Button>();
            button->set_icon_normal(
                    std::make_shared<VectorImage>(&app->get_context(), get_asset_dir("icons/Node_Button.svg")));
            button->container_sizing.flag_h = ContainerSizingFlag::ShrinkStart;
            vbox_container->add_child(button);
        }

        {
            auto check_button = std::make_shared<CheckButton>();
            check_button->container_sizing.flag_h = ContainerSizingFlag::ShrinkStart;
            vbox_container->add_child(check_button);

            auto callback = [](bool toggled) { Logger::info("Button toggled"); };
            check_button->connect_signal("toggled", callback);
        }

        {
            auto container_group = std::make_shared<VBoxContainer>();
            container_group->set_separation(8);
            container_group->set_position({10, 200});
            add_child(container_group);

            auto label = std::make_shared<Label>();
            label->set_text("Toggle Button Group");
            container_group->add_child(label);

            button_group = std::make_shared<ToggleButtonGroup>();

            for (int i = 0; i < 3; ++i) {
                auto check_button = std::make_shared<RadioButton>();
                check_button->container_sizing.flag_h = ContainerSizingFlag::ShrinkStart;
                container_group->add_child(check_button);
                button_group->add_button(check_button);
            }
        }
    }
};

// Process the next main command.
void handle_cmd(android_app *app_ctx, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_START:
            __android_log_print(ANDROID_LOG_INFO, "vecgui", "APP_CMD_START");
            break;
        case APP_CMD_RESUME:
            __android_log_print(ANDROID_LOG_INFO, "vecgui", "APP_CMD_RESUME");
            break;
        case APP_CMD_PAUSE:
            __android_log_print(ANDROID_LOG_INFO, "vecgui", "APP_CMD_PAUSE");
            break;
        case APP_CMD_STOP:
            __android_log_print(ANDROID_LOG_INFO, "vecgui", "APP_CMD_STOP");
            break;
        case APP_CMD_DESTROY:
            __android_log_print(ANDROID_LOG_INFO, "vecgui", "APP_CMD_DESTROY");
            break;
        case APP_CMD_INIT_WINDOW: {
            __android_log_print(ANDROID_LOG_INFO, "vecgui", "APP_CMD_INIT_WINDOW");

            AConfiguration *config = AConfiguration_new();
            AConfiguration_fromAssetManager(config, app_ctx->activity->assetManager);
            int32_t dpi = AConfiguration_getDensity(config);
            float dpi_scale = (float) dpi / DPI_STANDARD;
            AConfiguration_delete(config);
            __android_log_print(ANDROID_LOG_INFO, "vecgui",
                                "DPI: %d, Expected scale factor: %.1f", dpi, dpi_scale);

            auto window_size =
                    Pathfinder::Vec2I(ANativeWindow_getWidth(app_ctx->window),
                                      ANativeWindow_getHeight(app_ctx->window));

            app = new vecgui::App(app_ctx->window, app_ctx->activity->assetManager, window_size, USE_VULKAN);
            app->set_custom_scaling_factor(dpi_scale);

            app->get_tree_root()->add_child(std::make_shared<MyNode>());
        }
            break;
        case APP_CMD_TERM_WINDOW: {
            __android_log_print(ANDROID_LOG_INFO, "vecgui", "APP_CMD_TERM_WINDOW");

            app->single_run_cleanup();

            // The window is being hidden or closed or rotated, clean it up.
            delete app;
            app = nullptr;
        }
            break;
        default:
            __android_log_print(ANDROID_LOG_INFO, "vecgui", "Event not handled: %d", cmd);
    }
}

void handle_motion_event(GameActivityMotionEvent *event) {
    if (!app) {
        return;
    }

    int32_t actionMasked = event->action & AMOTION_EVENT_ACTION_MASK;
    int32_t pointerIndex = 0;

    // Only get pointer index for specific actions (DOWN/UP)
    if (actionMasked == AMOTION_EVENT_ACTION_POINTER_DOWN ||
        actionMasked == AMOTION_EVENT_ACTION_POINTER_UP) {
        pointerIndex =
                (event->action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                        >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
    }

    // Get X/Y coordinates for the affected pointer
    float x = GameActivityPointerAxes_getAxisValue(&event->pointers[pointerIndex],
                                                   AMOTION_EVENT_AXIS_X);
    float y = GameActivityPointerAxes_getAxisValue(&event->pointers[pointerIndex],
                                                   AMOTION_EVENT_AXIS_Y);

    float x_pos = x / app->get_scaling_factor();
    float y_pos = y / app->get_scaling_factor();

    auto input_server = app->get_context().input_server;

    switch (actionMasked) {
        case AMOTION_EVENT_ACTION_DOWN: {
            InputEvent input_event{};
            input_event.type = InputEventType::MouseButton;
            input_event.args.mouse_button.button = 0;
            input_event.args.mouse_button.pressed = true;
            input_event.args.mouse_button.position = {(float) x_pos, (float) y_pos};
            input_server->input_queue.push_back(input_event);
            input_server->cursor_position = {(float) x_pos, (float) y_pos};
        }
            break;
        case AMOTION_EVENT_ACTION_UP: {
            InputEvent input_event{};
            input_event.type = InputEventType::MouseButton;
            input_event.args.mouse_button.button = 0;
            input_event.args.mouse_button.pressed = false;
            input_event.args.mouse_button.position = {(float) x_pos, (float) y_pos};
            input_server->input_queue.push_back(input_event);

            input_server->cursor_position = {(float) x_pos, (float) y_pos};
        }
            break;
        case AMOTION_EVENT_ACTION_HOVER_MOVE:
        case AMOTION_EVENT_ACTION_MOVE: {
            InputEvent input_event{};
            input_event.type = InputEventType::MouseMotion;
            input_event.args.mouse_motion.position = {(float) x_pos, (float) y_pos};
            input_server->last_cursor_position = input_server->cursor_position;

            input_server->cursor_position = {(float) x_pos, (float) y_pos};

            input_event.args.mouse_motion.relative =
                    input_server->cursor_position - input_server->last_cursor_position;
            input_server->input_queue.push_back(input_event);
        }
            break;
        case AMOTION_EVENT_ACTION_SCROLL: {
            __android_log_print(ANDROID_LOG_INFO, "vecgui", "INPUT: SCROLL (%.1f, %.1f)", x, y);
        }
            break;
    }
}

void android_main(struct android_app *app_ctx) {
    // Set the callback to process system events.
    app_ctx->onAppCmd = handle_cmd;

    // In NativeActivity, you would do this: app->onInputEvent = engine_handle_input;
    // In GameActivity, you should NOT do this. The field is unused/removed for input.

    // It's also recommended to nullify any filters to ensure all input is received:
    android_app_set_key_event_filter(app_ctx, nullptr);
    android_app_set_motion_event_filter(app_ctx, nullptr);

    // Used to poll the events in the main loop.
    int events;
    android_poll_source *source;

    // Main loop.
    do {
        if (ALooper_pollOnce(0, nullptr, &events, (void **) &source) >= 0) {
            if (source != nullptr) {
                source->process(app_ctx, source);
            }
        }

        // 2. Get the current input buffer
        android_input_buffer *input_buffer = android_app_swap_input_buffers(app_ctx);

        // 3. Process Motion Events (Touch/Controller Analog)
        if (input_buffer && input_buffer->motionEventsCount) {
            for (int i = 0; i < input_buffer->motionEventsCount; i++) {
                handle_motion_event(&input_buffer->motionEvents[i]);
            }
            // Clear the motion events queue for the next frame
            android_app_clear_motion_events(input_buffer);
        }

        if (app) {
            app->single_run();
        }
    } while (app_ctx->destroyRequested == 0);
}
