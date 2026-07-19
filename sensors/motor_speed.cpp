#include "sensors/motor_speed.h"
#include "drivers/uart/uart_ringbuf.h"
#include "ti_msp_dl_config.h"

// ISR 在 namespace 外访问的文件作用域变量
static drivers::uart::RingBuf s_rb;

namespace sensors::motor_speed {

namespace {
Data   g_data = {};
bool   g_new  = false;

enum { S_IDLE, S_DOLLAR, S_M, S_S, S_P, S_D, S_COLON, S_DATA, S_END } g_st;
int16_t g_vals[4];
int     g_vi, g_di;
char    g_dbuf[8];
bool    g_neg;

void reset() {
    g_st = S_IDLE;
    g_vi = 0;
    g_di = 0;
    g_neg = false;
}
}  // namespace

void init() {
    s_rb.flush();
    g_data = {};
    g_new  = false;
    reset();
    NVIC_ClearPendingIRQ(DRIVE_UART_INST_INT_IRQN);
    NVIC_EnableIRQ(DRIVE_UART_INST_INT_IRQN);
}

Data read() {
    uint8_t tmp[64];
    while (true) {
        size_t n = s_rb.pop(tmp, sizeof(tmp));
        if (n == 0) break;
        for (size_t i = 0; i < n; ++i) {
            uint8_t b = tmp[i];
            switch (g_st) {
            case S_IDLE:   if (b == '$') g_st = S_DOLLAR; break;
            case S_DOLLAR: g_st = (b == 'M') ? S_M : S_IDLE; break;
            case S_M:      g_st = (b == 'S') ? S_S : S_IDLE; break;
            case S_S:      g_st = (b == 'P') ? S_P : S_IDLE; break;
            case S_P:      g_st = (b == 'D') ? S_D : S_IDLE; break;
            case S_D:      g_st = (b == ':') ? S_COLON : S_IDLE; break;
            case S_COLON:
                if (b == '#') { reset(); break; }
                if (b == ',') {
                    if (g_di) { int v=0; for(int j=0;j<g_di;j++)v=v*10+g_dbuf[j]-'0'; g_vals[g_vi]=g_neg?-v:v; g_vi++; }
                    g_di = 0; g_neg = false;
                } else if (b == '-') {
                    g_neg = true;
                } else if (b >= '0' && b <= '9' && g_di < 7) {
                    g_dbuf[g_di++] = b;
                }
                break;
            case S_END:
                if (g_vi == 3 && g_di) { int v=0; for(int j=0;j<g_di;j++)v=v*10+g_dbuf[j]-'0'; g_vals[3]=g_neg?-v:v; }
                g_data.m1 = g_vals[0]; g_data.m2 = g_vals[1];
                g_data.m3 = g_vals[2]; g_data.m4 = g_vals[3];
                g_data.fresh = true;
                g_new = true;
                reset();
                break;
            }
        }
    }
    if (g_new) { g_new = false; g_data.fresh = true; }
    else        g_data.fresh = false;
    return g_data;
}

void on_rx_byte(uint8_t b) { s_rb.push(b); }

}  // namespace sensors::motor_speed

extern "C" void DRIVE_UART_INST_IRQHandler(void) {
    if (DL_UART_Main_getPendingInterrupt(DRIVE_UART_INST) == DL_UART_MAIN_IIDX_RX) {
        while (!DL_UART_Main_isRXFIFOEmpty(DRIVE_UART_INST)) {
            sensors::motor_speed::on_rx_byte(DL_UART_Main_receiveData(DRIVE_UART_INST));
        }
    }
}