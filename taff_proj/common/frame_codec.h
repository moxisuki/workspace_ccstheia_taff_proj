// 通用帧编解码 (跨 UART 链路复用)
// 帧: [HEAD1][HEAD2][TYPE][LEN][payload...][XOR]
#pragma once
#include <stdint.h>
#include <stddef.h>

namespace common::frame {
constexpr uint8_t kHead1 = 0xAA, kHead2 = 0x55;
constexpr uint8_t kMaxPayload = 16, kOverhead = 5, kMaxFrame = kMaxPayload + kOverhead;
uint8_t xor8(uint8_t type, uint8_t len, const uint8_t* payload);
size_t  encode(uint8_t type, const uint8_t* payload, uint8_t len, uint8_t* out);

class Parser {
public:
    void reset();
    bool feed(uint8_t b, uint8_t& type, const uint8_t*& payload, uint8_t& len);
    uint32_t bad_xor_count()  const { return bad_xor_; }
    uint32_t overflow_count() const { return overflow_; }
private:
    enum State : uint8_t { S_HEAD1, S_HEAD2, S_TYPE, S_LEN, S_DATA, S_XOR };
    State    state_ = S_HEAD1;
    uint8_t  type_ = 0, len_ = 0, idx_ = 0, xor_acc_ = 0, buf_[kMaxPayload];
    uint32_t bad_xor_ = 0, overflow_ = 0;
};
}  // namespace common::frame