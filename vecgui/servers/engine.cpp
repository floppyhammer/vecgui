#include "engine.h"

#include <sstream>

#include "../common/utils.h"

namespace vecgui {

/// Period to print the frame time, in seconds.
constexpr float FRAME_TIME_PRINT_PERIOD = 5;

float remove_elements_less_than(std::map<int64_t, float>& my_map, const int64_t target_key) {
    float sum = 0;
    size_t count = 0;

    for (auto it = my_map.begin(); it != my_map.end();) {
        if (it->first < target_key) {
            // Erase the element and update the iterator to the next element
            it = my_map.erase(it);
        } else {
            sum += it->second;
            count++;
            // Move to the next element
            ++it;
        }
    }

    if (count == 0) {
        return sum;
    }

    return sum / (float)count;
}

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

    // Print frame time.
    // ----------------------------------------
    std::chrono::duration<double> duration = current_time - last_time_updated_fps;

    if (duration.count() > FRAME_TIME_PRINT_PERIOD) {
        // Show frame time.
        std::ostringstream string_stream;
        string_stream << "Frame time: " << round(dt * 1000.f * 100.f) * 0.01f << " ms";
        Logger::info(string_stream.str(), "vecgui");
        last_time_updated_fps = current_time;
    }
    // ----------------------------------------

    int64_t nanoseconds = current_time.time_since_epoch().count();
    frametimes[nanoseconds] = dt;
}

double Engine::get_dt() const {
    return dt;
}

double Engine::get_elapsed() const {
    return elapsed;
}

float Engine::get_fps() {
    auto current_time = std::chrono::steady_clock::now();
    int64_t nanoseconds = current_time.time_since_epoch().count();

    float average = remove_elements_less_than(frametimes, nanoseconds - fps_average_window);
    if (average == 0) {
        return 0;
    }

    return 1.0f / average;
}

int Engine::get_fps_int() {
    return int(round(get_fps()));
}

} // namespace vecgui
