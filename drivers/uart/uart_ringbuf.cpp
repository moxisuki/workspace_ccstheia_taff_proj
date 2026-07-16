#include "drivers/uart/uart_ringbuf.h"

namespace drivers::uart {

void RingBuf::push(uint8_t b) {
    uint16_t next = static_cast<uint16_t>((head_ + 1) % kSize);
    if (next != tail_) {
        buf_[head_] = b;
        head_ = next;
    }
}

size_t RingBuf::pop(uint8_t* dst, size_t max) {
    size_t n = 0;
    while (n < max && tail_ != head_) {
        dst[n++] = buf_[tail_];
        tail_ = static_cast<uint16_t>((tail_ + 1) % kSize);
    }
    return n;
}

}