#include "task/debug_task.h"

#include "common/print.h"
#include "control/heading/heading.h"
#include "task/heading_task.h"

namespace task::debug {

void init() {
    common::uart_print("[boot] taff_proj\r\n");
}

void loop(const sensors::state::State& s) {
    // Cube: roll, pitch, yaw
    common::uart_print_f1(s.roll);  common::uart_print(",");
    common::uart_print_f1(s.pitch); common::uart_print(",");
    common::uart_print_f1(s.yaw);   common::uart_print(",");
    // 控制: yr, err, turn
    common::uart_print_f1(s.yaw_rate); common::uart_print(",");
    common::uart_print_f1(control::heading::last_error()); common::uart_print(",");
    common::uart_print_f1(task::heading::last_turn());
    common::uart_println();
}

}  // namespace task::debug