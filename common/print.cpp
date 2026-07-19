#include "common/print.h"
#include "drivers/uart/uart.h"

namespace common {

static void write_char(char c) {
    drivers::uart::write(drivers::uart::Id::Debug, &c, 1);
}

void uart_print(const char* s) {
    while (*s) {
        write_char(*s);
        ++s;
    }
}

void uart_println() {
    write_char('\r');
    write_char('\n');
}

void uart_print_int(int32_t val) {
    char buf[12];
    int idx = 0;
    bool neg = false;
    if (val < 0) { neg = true; val = -val; }
    do {
        buf[idx++] = '0' + (val % 10);
        val /= 10;
    } while (val > 0);
    if (neg) buf[idx++] = '-';
    while (idx--) write_char(buf[idx]);
}

void uart_print_f1(float val) {
    int32_t int_part = (int32_t)val;
    int32_t dec_part = (int32_t)((val - int_part) * 10.0f);
    if (dec_part < 0) { dec_part = -dec_part; }
    uart_print_int(int_part);
    write_char('.');
    write_char('0' + (dec_part % 10));
}

}