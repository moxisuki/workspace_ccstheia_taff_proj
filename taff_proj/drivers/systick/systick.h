#pragma once
#include <stdint.h>

namespace drivers::systick {

void init();
uint32_t now_ms();
void delay_ms(uint32_t ms);

}