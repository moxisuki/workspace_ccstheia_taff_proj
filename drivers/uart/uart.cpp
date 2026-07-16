#include "drivers/uart/uart.h"
#include "ti_msp_dl_config.h"

namespace drivers::uart {

static UART_Regs* const kInst[] = {
    DEBUG_UART_INST,
    DRIVE_UART_INST,
    IMU_UART_INST,
    K230_UART_INST,
};

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