// 电机串口协议解析 — $MAll: encoder pulses → 差分算 RPM
// 这里是电驱板遥测: MCU 只用于调试/安全判断, 电机速度内环仍由电驱板 PID 完成
#include "sensors/motor_speed/motor_speed.h"
#include "common/vehicle_params.h"
#include "drivers/uart/uart.h"
#include "drivers/uart/uart_ringbuf.h"
#include "drivers/systick/systick.h"
#include "ti_msp_dl_config.h"

namespace sensors::motor_speed {
namespace {
constexpr int kPulsePerRev = common::vehicle_params::motor_board::kPulsePerRev;

drivers::uart::RingBuf g_rb;
volatile uint32_t g_rx_total = 0;
Data g_data = {}; bool g_new = false;

enum St { S_IDLE, S_DOLLAR, S_M, S_A, S_L1, S_L2, S_COLON };
St g_st = S_IDLE;

// 数值解析
int32_t g_raw_vals[4]; int g_vi;
int32_t g_val;
bool g_neg;

// 差分求速
int32_t g_last[4];
uint32_t g_last_ms;
bool g_has_last = false;

void reset_vals() { g_vi = 0; g_val = 0; g_neg = false; }

void emit_value() {
    int32_t v = g_neg ? -g_val : g_val;
    if (g_vi < 4) g_raw_vals[g_vi++] = v;
    g_val = 0; g_neg = false;
}

void compute_speed() {
    uint32_t now = drivers::systick::now_ms();
    // 先备份原始编码器值, 用于下次差分
    int32_t raw[4];
    for (int i = 0; i < 4; ++i) raw[i] = g_raw_vals[i];
    if (g_has_last) {
        float dt = (now - g_last_ms) / 1000.0f;
        if (dt > 0.005f && dt < 2.0f) {
            for (int i = 0; i < 4; ++i) {
                int32_t dp = raw[i] - g_last[i];
                // RPM = dp * 60 / (kPulsePerRev * dt)
                float rpm = (float)dp * 60.0f / ((float)kPulsePerRev * dt);
                if      (rpm >  999.0f) rpm =  999.0f;
                else if (rpm < -999.0f) rpm = -999.0f;
                g_raw_vals[i] = (int32_t)(rpm * 10.0f);  // 覆盖为 RPM ×10
            }
        }
    }
    // 存原始编码器值, 不做 RPM 覆盖
    for (int i = 0; i < 4; ++i) g_last[i] = raw[i];
    g_last_ms = now;
    g_has_last = true;
}

void commit() {
    g_data.m1=(int16_t)g_raw_vals[0]; g_data.m2=(int16_t)g_raw_vals[1];
    g_data.m3=(int16_t)g_raw_vals[2]; g_data.m4=(int16_t)g_raw_vals[3];
    g_data.fresh=true; g_new=true;
}
}  // namespace

void init() {
    g_rb.flush(); g_rx_total = 0; g_data = {}; g_new = false;
    g_st = S_IDLE; g_has_last = false; reset_vals();
    NVIC_ClearPendingIRQ(DRIVE_UART_INST_INT_IRQN);
    NVIC_EnableIRQ(DRIVE_UART_INST_INT_IRQN);
}

Data read() {
    uint8_t tmp[64];
    while (true) {
        size_t n = g_rb.pop(tmp, sizeof(tmp));
        if (n == 0) break;
        for (size_t i = 0; i < n; ++i) {
            uint8_t b = tmp[i];
            switch (g_st) {
            case S_IDLE:   if (b == '$') g_st = S_DOLLAR; break;
            case S_DOLLAR: g_st = (b == 'M') ? S_M : S_IDLE; break;
            case S_M:      g_st = (b == 'A') ? S_A : S_IDLE; break;
            case S_A:      g_st = (b == 'l') ? S_L1 : S_IDLE; break;
            case S_L1:     g_st = (b == 'l') ? S_L2 : S_IDLE; break;
            case S_L2:     if (b == ':') { g_st = S_COLON; reset_vals(); }
                           else g_st = S_IDLE; break;
            case S_COLON:
                if (b == '#') {
                    emit_value();
                    if (g_vi == 4) { compute_speed(); commit(); }
                    g_st = S_IDLE; break;
                }
                if (b == ',') { emit_value(); break; }
                if (b == '-') { g_neg = true; break; }
                if (b >= '0' && b <= '9') g_val = g_val * 10 + (int)(b - '0');
                break;
            }
        }
    }
    if (g_new) { g_new = false; g_data.fresh = true; }
    else g_data.fresh = false;
    return g_data;
}

void on_rx_byte(uint8_t b) { g_rx_total++; g_rb.push(b); }
uint32_t total_rx_bytes() { return g_rx_total; }
size_t peek_raw(uint8_t* buf, size_t max) { return g_rb.peek_recent(buf, max); }
}  // namespace sensors::motor_speed
