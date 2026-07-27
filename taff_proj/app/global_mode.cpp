#include "app/global_mode.h"

namespace app::global_mode {

namespace {
Snapshot g_state = { Mode::Stop, Mode::Stop, 0, 0 };

bool valid(Mode mode) {
    switch (mode) {
    case Mode::Stop:
    case Mode::LineTrace:
    case Mode::Straight:
    case Mode::UTurn:
        return true;
    }
    return false;
}
}  // namespace

void init() {
    g_state.current = Mode::Stop;
    g_state.previous = Mode::Stop;
    g_state.entered_ms = 0;
    g_state.change_count = 0;
}

Snapshot read() { return g_state; }

Mode mode() { return g_state.current; }

Mode previous_mode() { return g_state.previous; }

uint32_t entered_ms() { return g_state.entered_ms; }

uint32_t elapsed_ms(uint32_t now_ms) { return now_ms - g_state.entered_ms; }

uint32_t change_count() { return g_state.change_count; }

bool can_change(Mode from, Mode to) {
    if (!valid(from) || !valid(to)) return false;
    if (from == to) return true;
    if (to == Mode::Stop) return true;

    switch (from) {
    case Mode::Stop:
        return to == Mode::LineTrace || to == Mode::Straight || to == Mode::UTurn;
    case Mode::LineTrace:
        return to == Mode::Straight || to == Mode::UTurn;
    case Mode::Straight:
        return to == Mode::LineTrace || to == Mode::UTurn;
    case Mode::UTurn:
        return to == Mode::LineTrace || to == Mode::Straight;
    }
    return false;
}

bool change_mode(Mode next, uint32_t now_ms) {
    if (!can_change(g_state.current, next)) return false;
    if (g_state.current == next) return true;

    g_state.previous = g_state.current;
    g_state.current = next;
    g_state.entered_ms = now_ms;
    g_state.change_count++;
    return true;
}

const char* to_string(Mode mode) {
    switch (mode) {
    case Mode::Stop:      return "Stop";
    case Mode::LineTrace: return "LineTrace";
    case Mode::Straight:  return "Straight";
    case Mode::UTurn:     return "UTurn";
    }
    return "Unknown";
}

}  // namespace app::global_mode
