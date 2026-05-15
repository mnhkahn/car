#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLECharacteristic.h>
#include <BLE2902.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define ENA 5
#define IN1 6
#define IN2 7
#define ENB 8
#define IN3 9
#define IN4 10

// Onboard RGB LED (WS2812)
#define NEOPIXEL_PIN 48

// OLED SH1106
#define OLED_SDA 11
#define OLED_SCL 12
Adafruit_SH1106G display(128, 64, &Wire, -1);

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

// Current command for OLED display
char currentCmd = 'S';
char prevDisplayCmd = 0;

// LED state
unsigned long ledColorTime = 0;
uint32_t currentLedColor = 0;
const unsigned long LED_DURATION = 300;

uint32_t colorForCmd(char cmd) {
  switch (cmd) {
    case 'F': return Adafruit_NeoPixel::Color(0, 40, 0);
    case 'B': return Adafruit_NeoPixel::Color(40, 0, 0);
    case 'L': return Adafruit_NeoPixel::Color(0, 0, 40);
    case 'R': return Adafruit_NeoPixel::Color(40, 20, 0);
    case 'l': return Adafruit_NeoPixel::Color(0, 20, 20);
    case 'r': return Adafruit_NeoPixel::Color(20, 10, 0);
    case 'S': return Adafruit_NeoPixel::Color(0, 0, 0);
    default: return Adafruit_NeoPixel::Color(20, 20, 20);
  }
}

void drawSmiley() {
  display.clearDisplay();
  int cx = 64, cy = 32, r = 22;
  // Face outline
  display.drawCircle(cx, cy, r, SH110X_WHITE);
  // Left eye (arc, like ^_^)
  display.drawCircle(cx - 7, cy - 4, 4, SH110X_WHITE);
  display.fillCircle(cx - 7, cy - 4, 2, SH110X_BLACK);
  display.fillCircle(cx - 7, cy - 4, 1, SH110X_WHITE);
  // Right eye
  display.drawCircle(cx + 7, cy - 4, 4, SH110X_WHITE);
  display.fillCircle(cx + 7, cy - 4, 2, SH110X_BLACK);
  display.fillCircle(cx + 7, cy - 4, 1, SH110X_WHITE);
  // Blush (left cheek)
  display.fillCircle(cx - 14, cy + 4, 3, SH110X_WHITE);
  display.fillCircle(cx - 14, cy + 4, 1, SH110X_BLACK);
  // Blush (right cheek)
  display.fillCircle(cx + 14, cy + 4, 3, SH110X_WHITE);
  display.fillCircle(cx + 14, cy + 4, 1, SH110X_BLACK);
  // Smile (downward arc)
  for (int i = -8; i <= 8; i++) {
    int x = cx + i;
    int y = cy + 8 - (i * i) / 10;
    display.drawPixel(x, y, SH110X_WHITE);
    display.drawPixel(x, y + 1, SH110X_WHITE);
  }
  display.display();
}

void drawArrowUp() {
  display.clearDisplay();
  int cx = 64, cy = 32;
  display.fillRect(cx - 6, cy - 4, 12, 28, SH110X_WHITE);
  display.fillTriangle(cx, cy - 20, cx - 18, cy, cx + 18, cy, SH110X_WHITE);
  display.display();
}

void drawArrowDown() {
  display.clearDisplay();
  int cx = 64, cy = 32;
  display.fillRect(cx - 6, cy - 24, 12, 28, SH110X_WHITE);
  display.fillTriangle(cx, cy + 20, cx - 18, cy, cx + 18, cy, SH110X_WHITE);
  display.display();
}

void drawArrowLeft() {
  display.clearDisplay();
  int cx = 64, cy = 32;
  display.fillRect(cx - 4, cy - 6, 28, 12, SH110X_WHITE);
  display.fillTriangle(cx - 20, cy, cx, cy - 18, cx, cy + 18, SH110X_WHITE);
  display.display();
}

void drawArrowRight() {
  display.clearDisplay();
  int cx = 64, cy = 32;
  display.fillRect(cx - 24, cy - 6, 28, 12, SH110X_WHITE);
  display.fillTriangle(cx + 20, cy, cx, cy - 18, cx, cy + 18, SH110X_WHITE);
  display.display();
}

void updateDisplay() {
  char displayCmd = currentCmd;
  if (millis() - lastCmdTime > 500) displayCmd = 'S';
  if (displayCmd == prevDisplayCmd) return;
  prevDisplayCmd = displayCmd;

  switch (displayCmd) {
    case 'F': drawArrowUp(); break;
    case 'B': drawArrowDown(); break;
    case 'L': drawArrowLeft(); break;
    case 'R': drawArrowRight(); break;
    case 'l': drawArrowLeft(); break;
    case 'r': drawArrowRight(); break;
    default:  drawSmiley(); break;
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
      currentCmd = cmd;

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

  Wire.begin(11, 12);
  display.begin(0x3C, true);
  // If screen is blank, try 0x3D instead:
  // display.begin(0x3D, true);
  display.clearDisplay();
  drawSmiley();

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
  if (millis() - lastCmdTime > 500) {
    motor.stop();
    currentCmd = 'S';
  }

  updateDisplay();

  // RGB LED feedback
  if (!deviceConnected) {
    rgb.setPixelColor(0, (millis() / 500) % 2
      ? Adafruit_NeoPixel::Color(0, 0, 0)
      : Adafruit_NeoPixel::Color(10, 10, 10));
    rgb.show();
  } else if (millis() - ledColorTime < LED_DURATION) {
    rgb.setPixelColor(0, currentLedColor);
    rgb.show();
  } else {
    rgb.setPixelColor(0, Adafruit_NeoPixel::Color(0, 0, 0));
    rgb.show();
  }

  // Disconnect handling
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
