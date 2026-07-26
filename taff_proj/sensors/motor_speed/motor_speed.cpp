// 电机速度解析 (协议层) — 协议: $MAll:m1,m2,m3,m4#
#include "sensors/motor_speed/motor_speed.h"
#include "drivers/uart/uart_ringbuf.h"
#include "ti_msp_dl_config.h"

namespace sensors::motor_speed {
namespace {
drivers::uart::RingBuf g_rb;
volatile uint32_t g_rx_total = 0;
Data g_data = {}; bool g_new = false;
enum { S_IDLE, S_DOLLAR, S_M, S_A, S_L1, S_L2, S_COLON, S_END } g_st;
int16_t g_vals[4]; int g_vi, g_di; char g_dbuf[8]; bool g_neg;
void reset() { g_st = S_IDLE; g_vi = 0; g_di = 0; g_neg = false; }
}

void init() { g_rb.flush(); g_rx_total = 0; g_data = {}; g_new = false; reset();
    NVIC_ClearPendingIRQ(DRIVE_UART_INST_INT_IRQN); NVIC_EnableIRQ(DRIVE_UART_INST_INT_IRQN); }

Data read() {
    uint8_t tmp[64];
    while (true) { size_t n = g_rb.pop(tmp, sizeof(tmp)); if (n == 0) break;
        for (size_t i = 0; i < n; ++i) { uint8_t b = tmp[i];
            switch (g_st) {
            case S_IDLE: if (b == '$') g_st = S_DOLLAR; break;
            case S_DOLLAR: g_st = (b == 'M') ? S_M : S_IDLE; break;
            case S_M: g_st = (b == 'A') ? S_A : S_IDLE; break;
            case S_A: g_st = (b == 'l') ? S_L1 : S_IDLE; break;
            case S_L1: g_st = (b == 'l') ? S_L2 : S_IDLE; break;
            case S_L2: g_st = (b == ':') ? S_COLON : S_IDLE; break;
            case S_COLON:
                if (b == '#') { if (g_di) { int v=0; for(int j=0;j<g_di;j++)v=v*10+g_dbuf[j]-'0'; g_vals[g_vi]=g_neg?-v:v; }
                    g_data.m1=g_vals[0]; g_data.m2=g_vals[1]; g_data.m3=g_vals[2]; g_data.m4=g_vals[3];
                    g_data.fresh=true; g_new=true; reset(); break; }
                if (b == ',') { if (g_di) { int v=0; for(int j=0;j<g_di;j++)v=v*10+g_dbuf[j]-'0'; g_vals[g_vi]=g_neg?-v:v; g_vi++; }
                    g_di=0; g_neg=false; }
                else if (b == '-') g_neg = true;
                else if (b >= '0' && b <= '9' && g_di < 7) g_dbuf[g_di++] = b;
                break;
            case S_END: if (g_vi==3&&g_di) { int v=0; for(int j=0;j<g_di;j++)v=v*10+g_dbuf[j]-'0'; g_vals[3]=g_neg?-v:v; }
                g_data.m1=g_vals[0]; g_data.m2=g_vals[1]; g_data.m3=g_vals[2]; g_data.m4=g_vals[3];
                g_data.fresh=true; g_new=true; reset(); break;
            }
        }
    }
    if (g_new) { g_new=false; g_data.fresh=true; } else g_data.fresh=false;
    return g_data;
}
void on_rx_byte(uint8_t b) { g_rx_total++; g_rb.push(b); }
uint32_t total_rx_bytes() { return g_rx_total; }
size_t peek_raw(uint8_t* buf, size_t max) { return g_rb.peek_recent(buf, max); }
}  // namespace sensors::motor_speed