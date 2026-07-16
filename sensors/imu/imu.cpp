#include "sensors/imu/imu.h"
#include "drivers/uart/uart_ringbuf.h"
#include "drivers/systick/systick.h"
#include "ti_msp_dl_config.h"

namespace sensors::imu {

namespace {

constexpr uint8_t  kHead1    = 0x55;
constexpr uint8_t  kT_Accel  = 0x51;
constexpr uint8_t  kT_Gyro   = 0x52;
constexpr uint8_t  kT_Angle  = 0x53;
constexpr size_t   kDataLen  = 8;

drivers::uart::RingBuf g_rb;
Data                   g_data = {};
volatile bool          g_fault = false;
volatile bool          g_new_frame = false;

enum class State : uint8_t { Head1, Head2, Data, Sum };
State  g_state = State::Head1;
uint8_t g_type = 0;
uint8_t g_buf[kDataLen];
uint8_t g_idx = 0;
uint8_t g_sum = 0;

int16_t to_i16(uint8_t lo, uint8_t hi) {
    return static_cast<int16_t>((hi << 8) | lo);
}

void parse_frame() {
    int16_t v[3] = {
        to_i16(g_buf[0], g_buf[1]),
        to_i16(g_buf[2], g_buf[3]),
        to_i16(g_buf[4], g_buf[5])
    };
    int16_t t = to_i16(g_buf[6], g_buf[7]);
    float   tc = static_cast<float>(t) / 340.0f + 36.53f;

    switch (g_type) {
        case kT_Accel:
            g_data.ax = static_cast<float>(v[0]) / 32768.0f * 16.0f;
            g_data.ay = static_cast<float>(v[1]) / 32768.0f * 16.0f;
            g_data.az = static_cast<float>(v[2]) / 32768.0f * 16.0f;
            g_data.temperature = tc;
            break;
        case kT_Gyro:
            g_data.gx = static_cast<float>(v[0]) / 32768.0f * 2000.0f;
            g_data.gy = static_cast<float>(v[1]) / 32768.0f * 2000.0f;
            g_data.gz = static_cast<float>(v[2]) / 32768.0f * 2000.0f;
            g_data.temperature = tc;
            break;
        case kT_Angle:
            g_data.roll  = static_cast<float>(v[0]) / 32768.0f * 180.0f;
            g_data.pitch = static_cast<float>(v[1]) / 32768.0f * 180.0f;
            g_data.yaw   = static_cast<float>(v[2]) / 32768.0f * 180.0f;
            g_data.temperature = tc;
            break;
    }
    g_data.valid = true;
    g_new_frame = true;
}

void service_byte(uint8_t b) {
    g_sum = static_cast<uint8_t>(g_sum + b);
    switch (g_state) {
        case State::Head1:
            if (b == kHead1) { g_sum = kHead1; g_state = State::Head2; }
            break;
        case State::Head2:
            if ((b & 0xF0) == 0x50) {
                g_type = b;
                g_idx  = 0;
                g_state = State::Data;
            } else if (b == kHead1) {
                g_sum = kHead1;
            } else {
                g_state = State::Head1;
            }
            break;
        case State::Data:
            g_buf[g_idx++] = b;
            if (g_idx == kDataLen) g_state = State::Sum;
            break;
        case State::Sum:
            if (b == g_sum) parse_frame();
            else            g_fault = true;
            g_state = State::Head1;
            break;
    }
}

}  // namespace

void init() {
    g_rb.flush();
    g_data = {};
    g_fault = false;
    g_state = State::Head1;
    NVIC_ClearPendingIRQ(IMU_UART_INST_INT_IRQN);
    NVIC_EnableIRQ(IMU_UART_INST_INT_IRQN);
}

Data read() {
    uint8_t tmp[64];
    while (true) {
        size_t n = g_rb.pop(tmp, sizeof(tmp));
        if (n == 0) break;
        for (size_t i = 0; i < n; ++i) service_byte(tmp[i]);
    }
    if (g_new_frame) {
        g_data.fresh = true;
        g_data.timestamp_ms = drivers::systick::now_ms();
        g_new_frame = false;
    } else {
        g_data.fresh = false;
    }
    return g_data;
}

bool isFault() { return g_fault; }

void on_rx_byte(uint8_t b) { g_rb.push(b); }

}  // namespace sensors::imu

extern "C" void IMU_UART_INST_IRQHandler(void) {
    if (DL_UART_Main_getPendingInterrupt(IMU_UART_INST) == DL_UART_MAIN_IIDX_RX) {
        while (!DL_UART_Main_isRXFIFOEmpty(IMU_UART_INST)) {
            sensors::imu::on_rx_byte(DL_UART_Main_receiveData(IMU_UART_INST));
        }
    }
}