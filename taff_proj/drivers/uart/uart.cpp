#include "drivers/uart/uart.h"
#include "drivers/uart/uart_ringbuf.h"
#include "ti_msp_dl_config.h"

namespace drivers::uart {

static UART_Regs* const kInst[] = {
    DEBUG_UART_INST,
    DRIVE_UART_INST,
    IMU_UART_INST,
    K230_UART_INST,
};

RingBuf g_debug_tx;

void init() {}

size_t write(Id id, const void* data, size_t len) {
    auto* h = kInst[static_cast<uint8_t>(id)];
    const uint8_t* p = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < len; ++i) {
        while (DL_UART_Main_isTXFIFOFull(h)) {}
        DL_UART_Main_transmitData(h, p[i]);
    }
    return len;
}

size_t write_async(Id id, const void* data, size_t len) {
    if (id != Id::Debug) {
        return write(id, data, len);
    }

    const uint8_t* p = static_cast<const uint8_t*>(data);
    size_t written = 0;
    for (; written < len; ++written) {
        size_t before = g_debug_tx.available();
        g_debug_tx.push(p[written]);
        if (g_debug_tx.available() == before) break;
    }
    service_tx();
    return written;
}

void service_tx() {
    auto* h = DEBUG_UART_INST;
    uint8_t b;
    while (!DL_UART_Main_isTXFIFOFull(h)) {
        if (g_debug_tx.pop(&b, 1) != 1) break;
        DL_UART_Main_transmitData(h, b);
    }
}

size_t read(Id id, void* buf, size_t len) {
    auto* h = kInst[static_cast<uint8_t>(id)];
    uint8_t* p = static_cast<uint8_t*>(buf);
    size_t n = 0;
    while (n < len) {
        if (DL_UART_Main_isRXFIFOEmpty(h)) break;
        p[n++] = DL_UART_Main_receiveData(h);
    }
    return n;
}

bool readable(Id id) {
    return !DL_UART_Main_isRXFIFOEmpty(kInst[static_cast<uint8_t>(id)]);
}

}
