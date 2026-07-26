// 电机串口 ISR (硬件层)
#include "sensors/motor_speed/motor_speed.h"
#include "ti_msp_dl_config.h"
extern "C" void DRIVE_UART_INST_IRQHandler(void) {
    if (DL_UART_Main_getPendingInterrupt(DRIVE_UART_INST) == DL_UART_MAIN_IIDX_RX)
        while (!DL_UART_Main_isRXFIFOEmpty(DRIVE_UART_INST))
            sensors::motor_speed::on_rx_byte(DL_UART_Main_receiveData(DRIVE_UART_INST));
}