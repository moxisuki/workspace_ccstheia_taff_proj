#include "task/debug_task.h"

#include "common/print.h"
#include "control/heading/heading.h"

namespace task::debug {

void init() {
    common::uart_print("[boot] taff_proj\r\n");
}

void loop(const sensors::state::State& s) {
    common::uart_print_f1(s.roll);  common::uart_print(",");
    common::uart_print_f1(s.pitch); common::uart_print(",");
    common::uart_print_f1(s.yaw);   common::uart_print(",");
    common::uart_print_f1(s.yaw_rate); common::uart_print(",");
    common::uart_print_f1(control::heading::last_error()); common::uart_print(",");
    common::uart_print_int(s.m1); common::uart_print(",");
    common::uart_print_int(s.m2); common::uart_print(",");
    common::uart_print_int(s.m3); common::uart_print(",");
    common::uart_print_int(s.m4);
    common::uart_println();
}

}  // namespace task::debug
