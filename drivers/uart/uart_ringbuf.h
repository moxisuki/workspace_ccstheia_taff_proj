#pragma once
#include <stdint.h>
#include <stddef.h>

namespace drivers::uart {

class RingBuf {
public:
    static constexpr size_t kSize = 256;

    void push(uint8_t b);
    size_t pop(uint8_t* dst, size_t max);
    size_t available() const { return (head_ - tail_) % kSize; }
    void flush() { head_ = tail_ = 0; }

private:
    volatile uint8_t  buf_[kSize];
    volatile uint16_t head_ = 0;
    volatile uint16_t tail_ = 0;
};

}