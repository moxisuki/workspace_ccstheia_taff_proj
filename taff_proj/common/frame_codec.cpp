#include "common/frame_codec.h"
namespace common::frame {

uint8_t xor8(uint8_t type, uint8_t len, const uint8_t* payload) {
    uint8_t x = type ^ len;
    for (uint8_t i = 0; i < len; ++i) x ^= payload[i];
    return x;
}
size_t encode(uint8_t type, const uint8_t* payload, uint8_t len, uint8_t* out) {
    if (len > kMaxPayload) return 0;
    out[0] = kHead1; out[1] = kHead2; out[2] = type; out[3] = len;
    for (uint8_t i = 0; i < len; ++i) out[kOverhead - 1 + i] = payload[i];
    out[kOverhead - 1 + len] = xor8(type, len, payload);
    return static_cast<size_t>(kOverhead) + len;
}
void Parser::reset() { state_ = S_HEAD1; type_ = len_ = idx_ = xor_acc_ = 0; }
bool Parser::feed(uint8_t b, uint8_t& type, const uint8_t*& payload, uint8_t& len) {
    switch (state_) {
    case S_HEAD1: if (b == kHead1) state_ = S_HEAD2; return false;
    case S_HEAD2: if (b == kHead2) state_ = S_TYPE; else if (b == kHead1) {} else state_ = S_HEAD1; return false;
    case S_TYPE: type_ = b; xor_acc_ = b; state_ = S_LEN; return false;
    case S_LEN: len_ = b; xor_acc_ ^= b;
        if (b > kMaxPayload) { overflow_++; state_ = S_HEAD1; return false; }
        idx_ = 0; state_ = (b == 0) ? S_XOR : S_DATA; return false;
    case S_DATA: buf_[idx_++] = b; xor_acc_ ^= b; if (idx_ >= len_) state_ = S_XOR; return false;
    case S_XOR:
        if (b == xor_acc_) { type = type_; payload = buf_; len = len_; state_ = S_HEAD1; if (b == kHead1) state_ = S_HEAD2; return true; }
        else { bad_xor_++; state_ = S_HEAD1; return false; }
    }
    return false;
}
}  // namespace common::frame