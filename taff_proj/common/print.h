#pragma once
#include <stdint.h>

namespace common {

void uart_print(const char* s);
void uart_print_int(int32_t val);
void uart_print_f1(float val);
void uart_println();

}