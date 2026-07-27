#pragma once

#include <stdint.h>

namespace app::global_mode {

enum class Mode : uint8_t {
    Stop = 0,    // 停车等待
    LineTrace,   // 循迹
    Straight,    // 直线
    UTurn,       // 掉头
};

struct Snapshot {
    Mode current;
    Mode previous;
    uint32_t entered_ms;
    uint32_t change_count;
};

void init();

Snapshot read();
Mode mode();
Mode previous_mode();
uint32_t entered_ms();
uint32_t elapsed_ms(uint32_t now_ms);
uint32_t change_count();

bool can_change(Mode from, Mode to);
bool change_mode(Mode next, uint32_t now_ms);

const char* to_string(Mode mode);

}  // namespace app::global_mode
