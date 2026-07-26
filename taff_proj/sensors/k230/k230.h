#pragma once
#include <stdint.h>
#include "sensors/state/state.h"

namespace sensors::k230 {
enum FrameType : uint8_t {
    TYPE_TARGETS = 0x01, TYPE_HEARTBEAT = 0x02, TYPE_RESET = 0xFF,
    TYPE_ACK_OK = 0x81, TYPE_MODE_SET = 0x82, TYPE_RESET_ACK = 0x8F,
};
struct Link { uint32_t last_rx_ms = 0; };
void init();
sensors::state::Vision read();
void send_mode(uint8_t mode);
void send_ack(uint8_t seq);
void send_reset();
const Link& link();
uint32_t bad_xor_count();
uint32_t overflow_count();
}