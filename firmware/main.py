import bluetooth
import struct
from machine import Pin, PWM
import utime

# ===================== BLE Constants =====================
_IRQ_CENTRAL_CONNECT    = const(1)
_IRQ_CENTRAL_DISCONNECT = const(2)
_IRQ_GATTS_WRITE        = const(3)

_UART_SERVICE_UUID = bluetooth.UUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E")
_UART_RX_CHAR_UUID = bluetooth.UUID("6E400002-B5A3-F393-E0A9-E50E24DCCA9E")
_UART_TX_CHAR_UUID = bluetooth.UUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9E")

# ===================== Motor Pins =====================
ENA = 5
IN1 = 6
IN2 = 7
ENB = 8
IN3 = 9
IN4 = 10

# ===================== Advertising Payload Builder =====================
def advertising_payload(name=None, services=None):
    payload = bytearray()
    def _append(adv_type, value):
        nonlocal payload
        payload += struct.pack("BB", len(value) + 1, adv_type) + value

    _append(0x01, struct.pack("B", 0x06))  # General discoverable, BR/EDR not supported

    if name:
        _append(0x09, name.encode())

    if services:
        for uuid in services:
            b = bytes(uuid)
            if len(b) == 16:
                _append(0x07, b)
    return payload

# ===================== Motor Controller =====================
class MotorController:
    def __init__(self):
        self.pwm_a = PWM(Pin(ENA), freq=20000, duty=0)
        self.pwm_b = PWM(Pin(ENB), freq=20000, duty=0)
        self.in1 = Pin(IN1, Pin.OUT, value=0)
        self.in2 = Pin(IN2, Pin.OUT, value=0)
        self.in3 = Pin(IN3, Pin.OUT, value=0)
        self.in4 = Pin(IN4, Pin.OUT, value=0)
        self.speed = 200
        self._stopped = True

    def _set(self, pwm, speed):
        d = min(1023, max(0, int(speed * 4.012)))
        pwm.duty(d)

    def stop(self):
        if not self._stopped:
            self.in1.value(0); self.in2.value(0)
            self.in3.value(0); self.in4.value(0)
            self.pwm_a.duty(0); self.pwm_b.duty(0)
            self._stopped = True

    def forward(self, s):
        self.in1.value(1); self.in2.value(0)
        self.in3.value(1); self.in4.value(0)
        self._set(self.pwm_a, s); self._set(self.pwm_b, s)
        self._stopped = False

    def backward(self, s):
        self.in1.value(0); self.in2.value(1)
        self.in3.value(0); self.in4.value(1)
        self._set(self.pwm_a, s); self._set(self.pwm_b, s)
        self._stopped = False

    def turn_left(self, s):
        self.in1.value(0); self.in2.value(1)
        self.in3.value(1); self.in4.value(0)
        self._set(self.pwm_a, s); self._set(self.pwm_b, s)
        self._stopped = False

    def turn_right(self, s):
        self.in1.value(1); self.in2.value(0)
        self.in3.value(0); self.in4.value(1)
        self._set(self.pwm_a, s); self._set(self.pwm_b, s)
        self._stopped = False

    def spin_left(self, s):
        self.in1.value(1); self.in2.value(0)
        self.in3.value(1); self.in4.value(0)
        self._set(self.pwm_a, s // 3); self._set(self.pwm_b, s)
        self._stopped = False

    def spin_right(self, s):
        self.in1.value(1); self.in2.value(0)
        self.in3.value(1); self.in4.value(0)
        self._set(self.pwm_a, s); self._set(self.pwm_b, s // 3)
        self._stopped = False

# ===================== BLE UART Service =====================
class BLEUART:
    def __init__(self, ble, name):
        self._ble = ble
        self._ble.active(True)
        self._ble.irq(self._irq)

        ((self._handle_tx, self._handle_rx),) = self._ble.gatts_register_services((
            (_UART_SERVICE_UUID, (
                (_UART_TX_CHAR_UUID, bluetooth.FLAG_NOTIFY,),
                (_UART_RX_CHAR_UUID, bluetooth.FLAG_WRITE,),
            )),
        ))

        self.connected = False
        self._write_callback = None
        self._advertise(name)

    def _irq(self, event, data):
        if event == _IRQ_CENTRAL_CONNECT:
            self.connected = True
            print("BLE: Client connected")
        elif event == _IRQ_CENTRAL_DISCONNECT:
            self.connected = False
            print("BLE: Client disconnected")
            self._advertise("ESP32-S3-Car")
        elif event == _IRQ_GATTS_WRITE:
            _, value_handle = data
            if value_handle == self._handle_rx and self._write_callback:
                self._write_callback(self._ble.gatts_read(value_handle))

    def _advertise(self, name):
        # Keep adv_data under 31 bytes; name + flags only
        payload = advertising_payload(name=name)
        self._ble.gap_advertise(100000, adv_data=payload)

    def on_write(self, callback):
        self._write_callback = callback

# ===================== Main =====================
motors = MotorController()
ble = bluetooth.BLE()
uart = BLEUART(ble, "ESP32-S3-Car")
last_cmd_time = utime.ticks_ms()
was_stopped = True

def on_rx(data):
    global last_cmd_time
    last_cmd_time = utime.ticks_ms()

    if not data:
        return

    c = chr(data[0])

    # Speed: '0'-'9' -> 0-255
    if '0' <= c <= '9':
        motors.speed = int((ord(c) - 48) * 255 / 9)
        print("Speed set to", motors.speed)
        return

    # Optional inline speed, e.g. F180
    s = motors.speed
    if len(data) > 1:
        try:
            val = int(data[1:].decode())
            if 0 < val <= 255:
                s = val
        except:
            pass

    if c in ('F', 'f'):
        motors.forward(s); print("Forward", s)
    elif c in ('B', 'b'):
        motors.backward(s); print("Backward", s)
    elif c == 'L':
        motors.turn_left(s); print("Turn left", s)
    elif c == 'R':
        motors.turn_right(s); print("Turn right", s)
    elif c == 'l':
        motors.spin_left(s); print("Spin left", s)
    elif c == 'r':
        motors.spin_right(s); print("Spin right", s)
    elif c in ('S', 's'):
        motors.stop(); print("Stop")

uart.on_write(on_rx)
print("ESP32-S3 BLE Car ready. Advertising as 'ESP32-S3-Car'")
print("Commands: F/B/L/R/l/r/S, 0-9 for speed")

while True:
    now = utime.ticks_ms()
    if not uart.connected or utime.ticks_diff(now, last_cmd_time) > 500:
        if not was_stopped:
            motors.stop()
            was_stopped = True
    else:
        was_stopped = False
    utime.sleep_ms(50)
