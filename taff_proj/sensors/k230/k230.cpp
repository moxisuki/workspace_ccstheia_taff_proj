// K230 视觉驱动 (UART3) — 协议编解码由 common::frame 提供
#include "sensors/k230/k230.h"
#include "common/frame_codec.h"
#include "drivers/systick/systick.h"
#include "drivers/uart/uart.h"
#include "drivers/uart/uart_ringbuf.h"
#include "ti_msp_dl_config.h"

namespace sensors::k230 {
namespace {
constexpr uint32_t kStaleMs = 1000;
drivers::uart::RingBuf g_rx;
drivers::uart::RingBuf g_tx;
common::frame::Parser g_parser;
sensors::state::Vision g_vision = {};
uint8_t g_seq = 0;
Link g_link = {};

void service_tx() {
    uint8_t b;
    while (!DL_UART_Main_isTXFIFOFull(K230_UART_INST)) {
        if (g_tx.pop(&b, 1) != 1) break;
        DL_UART_Main_transmitData(K230_UART_INST, b);
    }
}

bool tx_send(uint8_t type, const uint8_t* payload, uint8_t len) {
    uint8_t buf[common::frame::kMaxFrame];
    size_t n = common::frame::encode(type, payload, len, buf);
    if (n == 0) return false;
    if (n > drivers::uart::RingBuf::kSize - 1U - g_tx.available()) return false;
    for (size_t i = 0; i < n; ++i) g_tx.push(buf[i]);
    service_tx();
    return true;
}
void on_target(const uint8_t* p, uint8_t l) {
    if (l < 4) return;
    g_vision.x = (float)(int16_t)(p[0]|(p[1]<<8));
    g_vision.y = (float)(int16_t)(p[2]|(p[3]<<8));
    g_vision.type = (l>=5)?p[4]:1; g_vision.flag = (l>=6)?p[5]:0;
    g_vision.valid = true; g_vision.fresh = true;
    g_link.last_rx_ms = drivers::systick::now_ms();
    send_ack(++g_seq);
}
void on_heartbeat() {
    g_vision.heartbeat_cnt++; g_vision.valid = true; g_vision.fresh = true;
    g_link.last_rx_ms = drivers::systick::now_ms();
    send_ack(++g_seq);
}
void on_reset() {
    g_vision = {};
    g_link = {};
    g_seq = 0;
    g_tx.flush();
    tx_send(TYPE_RESET_ACK, nullptr, 0);
}
void on_frame(uint8_t type, const uint8_t* payload, uint8_t len) {
    switch (type) {
    case TYPE_HEARTBEAT: on_heartbeat(); break;
    case TYPE_TARGETS: on_target(payload, len); break;
    case TYPE_RESET: on_reset(); break;
    default: break;
    }
}
}
extern "C" void K230_UART_INST_IRQHandler(void) {
    auto* h = K230_UART_INST;
    while (!DL_UART_Main_isRXFIFOEmpty(h)) g_rx.push(DL_UART_Main_receiveData(h));
}
void init() {
    g_rx.flush(); g_tx.flush(); g_parser.reset(); g_vision = {}; g_seq = 0; g_link = {};
    g_link.last_rx_ms = drivers::systick::now_ms();
    NVIC_ClearPendingIRQ(K230_UART_INST_INT_IRQN); NVIC_EnableIRQ(K230_UART_INST_INT_IRQN);
}
sensors::state::Vision read() {
    service_tx();
    uint8_t b;
    while (g_rx.pop(&b, 1) == 1) {
        uint8_t type; const uint8_t* payload; uint8_t len;
        if (g_parser.feed(b, type, payload, len)) on_frame(type, payload, len);
    }
    service_tx();
    g_vision.bad_xor = g_parser.bad_xor_count();
    g_vision.overflow = g_parser.overflow_count();
    if (drivers::systick::now_ms() - g_link.last_rx_ms > kStaleMs) g_vision.valid = false;

    sensors::state::Vision out = g_vision;
    g_vision.fresh = false;
    return out;
}
void send_mode(uint8_t mode) { tx_send(TYPE_MODE_SET, &mode, 1); }
void send_ack(uint8_t seq) { tx_send(TYPE_ACK_OK, &seq, 1); }
void send_reset() { tx_send(TYPE_RESET, nullptr, 0); }
const Link& link() { return g_link; }
uint32_t bad_xor_count() { return g_parser.bad_xor_count(); }
uint32_t overflow_count() { return g_parser.overflow_count(); }
}
