#include "engine.h"

#include <sstream>

#include "../common/utils.h"

namespace vecgui {

/// Period to print the frame time, in seconds.
static constexpr float FRAME_TIME_PRINT_PERIOD = 5;

Engine::Engine() {
    last_time_updated_fps = std::chrono::steady_clock::now();
}

void Engine::tick() {
    static auto start_time = std::chrono::steady_clock::now();

    auto current_time = std::chrono::steady_clock::now();

    auto new_elapsed = std::chrono::duration<double>(current_time - start_time).count();

    if (elapsed == 0) {
        elapsed = new_elapsed;
        return;
    }

    dt = new_elapsed - elapsed;
    elapsed = new_elapsed;

    // Update smoothed FPS using Exponential Moving Average (EMA).
    // Alpha (0.05) controls the smoothing; lower is smoother, higher is more responsive.
    if (dt > 0) {
        float instant_fps = static_cast<float>(1.0 / dt);
        if (smoothed_fps == 0.0f) {
            smoothed_fps = instant_fps;
        } else {
            constexpr float alpha = 0.05f;
            smoothed_fps = instant_fps * alpha + smoothed_fps * (1.0f - alpha);
        }
    }

    // Print frame time periodically.
    std::chrono::duration<double> duration = current_time - last_time_updated_fps;
    if (duration.count() > FRAME_TIME_PRINT_PERIOD) {
        std::ostringstream string_stream;
        string_stream << "Frame time: " << round(dt * 1000.f * 100.f) * 0.01f << " ms | FPS: " << get_fps_int();
        Logger::info(string_stream.str(), "vecgui");
        last_time_updated_fps = current_time;
    }
}

double Engine::get_dt() const {
    return dt;
}

double Engine::get_elapsed() const {
    return elapsed;
}

float Engine::get_fps() {
    return smoothed_fps;
}

int32_t Engine::get_fps_int() {
    return int32_t(round(smoothed_fps));
}

} // namespace vecgui
