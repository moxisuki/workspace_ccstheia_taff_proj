#include "drivers/uart/uart.h"
#include <stdio.h>

// 给DAP-LINK用的调试输出
extern "C" int fputc(int c, FILE* fp) {
    uint8_t b = static_cast<uint8_t>(c);
    drivers::uart::write(drivers::uart::Id::Debug, &b, 1);
    return c;
}
