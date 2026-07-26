# K230 ↔ MSPM0G3507 串口 util (CanMV MicroPython)
#
# 帧: [0xAA][0x55][TYPE][LEN][payload...][XOR]
# 校验 = TYPE ^ LEN ^ payload 各字节 (单字节)
#
# 用法:
#     proto = UARTProto()
#     proto.init()
#     proto.send_target(cx, cy, type_id, flag)        # 发目标
#     proto.send_heartbeat()                          # 发心跳 (MSPM0 会回 ACK)
#     proto.poll()                                    # 收非 ACK 帧
#     proto.check_link()                              # 判心跳是否丢

from machine import FPIOA, UART
import time as _time

# ── 默认硬件 (K230 UART2 = 大核 ↔ MSPM0) ───────────────────
#
# UART2 默认: GPIO11(TX)/GPIO12(RX) (跟之前 PROTOCOL.md 一致)
# UART0 被小核 (sh) 占用做大核 console, 大核 MicroPython 暴露 UART1/2/4. 选 UART2.
# UART1 (GPIO 3/4) 也可, 但要避开 IDE 默认占的 sensor(2) 相关 FPIOA 配置.
UART_TX_PIN   = 11
UART_TX_FUNC  = FPIOA.UART2_TXD
UART_RX_PIN   = 12
UART_RX_FUNC  = FPIOA.UART2_RXD
UART_ID       = UART.UART2
UART_BAUDRATE = 115200

# ── 协议常量 ───────────────────────────────────────────────

FRAME_HEAD1 = 0xAA
FRAME_HEAD2 = 0x55
FRAME_MAX_PAYLOAD = 16
TARGET_PKT_BYTES  = 6     # int16 x + int16 y + uint8 type + uint8 flag

TYPE_TARGETS    = 0x01
TYPE_HEARTBEAT  = 0x02
TYPE_RESET      = 0xFF
TYPE_ACK        = 0x81
TYPE_MODE       = 0x82
TYPE_RESET_ACK  = 0x8F

# Vision.flag 位定义
FLAG_CONFIRMED = 0x01   # bit0 = 目标确认
FLAG_TRACKED   = 0x02   # bit1 = 跟踪中


# ── 帧编码 ──────────────────────────────────────────────────

def _xor(typ, payload):
    x = typ ^ len(payload)
    for b in payload:
        x ^= b
    return x & 0xFF


def make_frame(typ, payload=b''):
    """组帧. payload 限 16 字节."""
    payload = bytes(payload)
    if len(payload) > FRAME_MAX_PAYLOAD:
        raise ValueError("payload > %d bytes" % FRAME_MAX_PAYLOAD)
    out = bytearray(5 + len(payload))
    out[0] = FRAME_HEAD1
    out[1] = FRAME_HEAD2
    out[2] = typ & 0xFF
    out[3] = len(payload) & 0xFF
    out[4:4 + len(payload)] = payload
    out[4 + len(payload)] = _xor(typ, payload)
    return bytes(out)


def encode_target(cx, cy, type_id=1, flag=0):
    """单目标 → 6 字节 (小端)."""
    p = bytearray(TARGET_PKT_BYTES)
    p[0] = int(cx) & 0xFF
    p[1] = (int(cx) >> 8) & 0xFF
    p[2] = int(cy) & 0xFF
    p[3] = (int(cy) >> 8) & 0xFF
    p[4] = int(type_id) & 0xFF
    p[5] = int(flag) & 0xFF
    return bytes(p)


# ── UART 封装 ───────────────────────────────────────────────

class UARTProto:
    def __init__(self, uart_id=UART_ID, baudrate=UART_BAUDRATE,
                 tx_pin=UART_TX_PIN, tx_func=UART_TX_FUNC,
                 rx_pin=UART_RX_PIN, rx_func=UART_RX_FUNC,
                 ack_timeout_ms=2000):
        self.uart_id   = uart_id
        self.baudrate  = baudrate
        self.tx_pin    = tx_pin
        self.rx_pin    = rx_pin
        self.tx_func   = tx_func
        self.rx_func   = rx_func
        self.ack_timeout_ms = ack_timeout_ms
        self.fpioa = None
        self.uart  = None
        self._rxbuf   = bytearray()
        self.acked_count  = 0
        self.missed_count = 0
        self._last_acked  = 0
        self._last_ms = 0

    def init(self):
        """FPIOA 配引脚 + UART 打开. 用 poll() 收,不用 ISR."""
        self.fpioa = FPIOA()
        self.fpioa.set_function(self.tx_pin, self.tx_func)
        self.fpioa.set_function(self.rx_pin, self.rx_func)
        self.uart = UART(self.uart_id, self.baudrate)
        self._last_ms = _time.ticks_ms()

    def deinit(self):
        if self.uart is not None:
            self.uart.deinit()
            self.uart = None

    def _send_raw(self, data):
        try:
            if self.uart is None:
                return
            if isinstance(data, str):
                self.uart.write(data)
            elif isinstance(data, (bytes, bytearray, list)):
                self.uart.write(bytes(data))
        except Exception as e:
            print("UART send err:", e)

    def send(self, data):
        """发 str / bytes / list."""
        self._send_raw(data)

    def send_target(self, cx, cy, type_id=1, flag=0):
        """发一目标. cx/cy 是像素坐标."""
        self._send_raw(make_frame(
            TYPE_TARGETS, encode_target(cx, cy, type_id, flag)))

    def send_reset(self):
        self._send_raw(make_frame(TYPE_RESET))

    def send_heartbeat(self):
        """发心跳 (空 payload). MSPM0 收到会回 ACK."""
        self._send_raw(make_frame(TYPE_HEARTBEAT))

    def poll(self):
        """非阻塞. 收一帧,返回 (typ, payload) 或 None.
           TYPE_ACK 帧会被静默统计,不返回."""
        try:
            n = self.uart.any()
            if n:
                chunk = self.uart.read(n)
                if chunk:
                    self._rxbuf.extend(chunk)
        except Exception as e:
            print("UART read err:", e)
            return None

        while len(self._rxbuf) >= 5:
            i = self._rxbuf.find(b'\xAA\x55')
            if i < 0:
                self._rxbuf = bytearray()
                return None
            if i > 0:
                self._rxbuf = self._rxbuf[i:]
            # 边界保护: LEN 字段截断到 kMaxPayload, 避免 corrupt 字节导致越界
            ln = min(self._rxbuf[3], FRAME_MAX_PAYLOAD) if len(self._rxbuf) > 3 else 0
            if len(self._rxbuf) < 5 + ln:
                return None
            try:
                typ     = self._rxbuf[2]
                payload = bytes(self._rxbuf[4:4 + ln])
                xor     = self._rxbuf[4 + ln]
                self._rxbuf = self._rxbuf[5 + ln:]
            except (IndexError, ValueError):
                self._rxbuf = self._rxbuf[1:]  # 跳过坏字节, 继续找
                continue
            if xor != _xor(typ, payload):
                continue
            if typ == TYPE_ACK:
                self.acked_count += 1
                continue
            return (typ, payload)
        return None

    def check_link(self):
        """心跳节拍检查. 主循环每 1 个心跳周期调用一次:
           - 调用时, 比较 acked_count 是否在前一周期内增长
           - 增长 → 清零 helper, 返回 True
           - 未增长 → 返回 False (本节拍丢失)
           同时检查超过 ack_timeout_ms 没发心跳 → 返回 None (没到要判的时候)

        用法:
            if proto.check_link() == False:
                # 链路断开, 可重连
        """
        now = _time.ticks_ms()
        diff = _time.ticks_diff(now, self._last_ms)
        if diff < self.ack_timeout_ms:
            return None

        cur_acked = self.acked_count
        if cur_acked > self._last_acked:
            self._last_acked = cur_acked
            return True
        self.missed_count += 1
        return False

    @staticmethod
    def parse_target(payload):
        """payload → (cx, cy, type, flag) 或 None."""
        if len(payload) < 6:
            return None
        cx = payload[0] | (payload[1] << 8)
        if cx & 0x8000: cx -= 0x10000
        cy = payload[2] | (payload[3] << 8)
        if cy & 0x8000: cy -= 0x10000
        return (cx, cy, payload[4], payload[5])
