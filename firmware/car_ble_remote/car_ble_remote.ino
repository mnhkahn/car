#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLECharacteristic.h>
#include <BLE2902.h>
#include <Adafruit_NeoPixel.h>

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define ENA 5
#define IN1 6
#define IN2 7
#define ENB 8
#define IN3 9
#define IN4 10

// Onboard RGB LED (WS2812) — most ESP32-S3 boards use GPIO 48
#define NEOPIXEL_PIN 48

class MotorDriver {
public:
  void init() {
    ledcAttach(ENA, 1000, 8);
    ledcAttach(ENB, 1000, 8);
    pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
    stop();
  }
  void set(int left, int right) {
    digitalWrite(IN1, left > 0); digitalWrite(IN2, left < 0);
    digitalWrite(IN3, right > 0); digitalWrite(IN4, right < 0);
    ledcWrite(ENA, constrain(abs(left), 0, 255));
    ledcWrite(ENB, constrain(abs(right), 0, 255));
  }
  void stop() {
    digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
    ledcWrite(ENA, 0); ledcWrite(ENB, 0);
  }
};

MotorDriver motor;
Adafruit_NeoPixel rgb(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
unsigned long lastCmdTime = 0;
int speed = 200;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// LED state
unsigned long ledColorTime = 0;
uint32_t currentLedColor = 0;
const unsigned long LED_DURATION = 300;  // ms

// Direction → color mapping
uint32_t colorForCmd(char cmd) {
  switch (cmd) {
    case 'F': return Adafruit_NeoPixel::Color(0, 40, 0);     // Green
    case 'B': return Adafruit_NeoPixel::Color(40, 0, 0);     // Red
    case 'L': return Adafruit_NeoPixel::Color(0, 0, 40);     // Blue
    case 'R': return Adafruit_NeoPixel::Color(40, 20, 0);    // Orange
    case 'l': return Adafruit_NeoPixel::Color(0, 20, 20);    // Cyan
    case 'r': return Adafruit_NeoPixel::Color(20, 10, 0);    // Gold
    case 'S': return Adafruit_NeoPixel::Color(0, 0, 0);      // Off
    default: return Adafruit_NeoPixel::Color(20, 20, 20);    // White (speed)
  }
}

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
  }
  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0) {
      char cmd = rxValue[0];
      lastCmdTime = millis();
      ledColorTime = millis();
      currentLedColor = colorForCmd(cmd);

      switch (cmd) {
        case 'F': motor.set(speed, speed); break;
        case 'B': motor.set(-speed, -speed); break;
        case 'L': motor.set(-speed, speed); break;
        case 'R': motor.set(speed, -speed); break;
        case 'l': motor.set(0, speed); break;
        case 'r': motor.set(speed, 0); break;
        case 'S': motor.stop(); break;
        default:
          if (cmd >= '0' && cmd <= '9') speed = map(cmd - '0', 0, 9, 50, 255);
          break;
      }
    }
  }
};

void setup() {
  rgb.begin();
  rgb.clear();
  rgb.show();

  motor.init();

  BLEDevice::init("ESP32-S3-Car");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pTxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();
  pServer->getAdvertising()->start();
}

void loop() {
  if (millis() - lastCmdTime > 500) motor.stop();

  // RGB LED feedback
  if (!deviceConnected) {
    // Disconnected: slow blink
    rgb.setPixelColor(0, (millis() / 500) % 2
      ? Adafruit_NeoPixel::Color(0, 0, 0)
      : Adafruit_NeoPixel::Color(10, 10, 10));
    rgb.show();
  } else if (millis() - ledColorTime < LED_DURATION) {
    // Show direction color
    rgb.setPixelColor(0, currentLedColor);
    rgb.show();
  } else {
    // Off
    rgb.setPixelColor(0, Adafruit_NeoPixel::Color(0, 0, 0));
    rgb.show();
  }

  // Disconnect handling: restart advertising
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    oldDeviceConnected = false;
  }
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = true;
  }

  delay(20);
}
