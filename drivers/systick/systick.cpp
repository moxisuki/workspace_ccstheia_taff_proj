#include "drivers/systick/systick.h"
#include "ti_msp_dl_config.h"

namespace drivers::systick {

volatile uint32_t g_tick_ms = 0;

void init() {
    g_tick_ms = 0;
    SysTick_Config(32000);  // 32MHz / 32000 = 1kHz
}

uint32_t now_ms() { return g_tick_ms; }

void delay_ms(uint32_t ms) {
    uint32_t start = g_tick_ms;
    while ((g_tick_ms - start) < ms) { __WFI(); }
}

}  // namespace drivers::systick

extern "C" void SysTick_Handler(void) {
    drivers::systick::g_tick_ms++;
}