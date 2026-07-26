from machine import FPIOA
from machine import UART

# UART2: TX=11, RX=12
UART_TX_PIN = 11
UART_TX_FUNC = FPIOA.UART2_TXD
UART_RX_PIN = 12
UART_RX_FUNC = FPIOA.UART2_RXD
UART_ID = UART.UART2
UART_BAUDRATE = 115200


class UartComm:
    def __init__(self, uart_id=UART_ID, baudrate=UART_BAUDRATE,
                 tx_pin=UART_TX_PIN, tx_func=UART_TX_FUNC,
                 rx_pin=UART_RX_PIN, rx_func=UART_RX_FUNC):
        self.uart_id = uart_id
        self.baudrate = baudrate
        self.tx_pin = tx_pin
        self.rx_pin = rx_pin
        self.tx_func = tx_func
        self.rx_func = rx_func
        self.fpioa = None
        self.uart = None

    def init(self):
        self.fpioa = FPIOA()
        self.fpioa.set_function(self.tx_pin, self.tx_func)
        self.fpioa.set_function(self.rx_pin, self.rx_func)
        self.uart = UART(self.uart_id, self.baudrate)
        print("UART initialized: id={}, baudrate={}, TX={}, RX={}".format(
            self.uart_id, self.baudrate, self.tx_pin, self.rx_pin))

    def send(self, data):
        try:
            """发送数据，支持 str 或 bytes"""
            if self.uart is None:
                return
            if isinstance(data, str):
                self.uart.write(data)

            elif isinstance(data, (bytes, bytearray, list)):
                self.uart.write(bytes(data))
        except:
            print("ERROR")

            

    def send_classification(self, label, cls_idx, score):
        """发送分类识别结果，格式: 'CLS,idx,score,label\\n'"""
        msg = "CLS,{:.4f},{}\n".format(score, label)
        self.send(msg)

    def read(self):
        """读取串口接收到的数据"""
        if self.uart is None:
            return None
        return self.uart.read()

    def deinit(self):
        if self.uart is not None:
            self.uart.deinit()
            self.uart = None
            print("UART deinitialized")
