#include <Arduino.h>
#include <NimBLEDevice.h>

// ===================== BLE UUIDs (Nordic UART Service) =====================
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// ===================== Pin Definitions (ESP32-S3) =====================
// Left motors (two motors in parallel -> L298N Channel A)
const int ENA = 5;   // PWM speed
const int IN1 = 6;   // Direction
const int IN2 = 7;   // Direction

// Right motors (two motors in parallel -> L298N Channel B)
const int ENB = 8;   // PWM speed
const int IN3 = 9;   // Direction
const int IN4 = 10;  // Direction

// Optional status LED
const int LED_PIN = 2;

// ===================== PWM Config =====================
const int PWM_FREQ = 20000;      // 20 kHz (above audible range)
const int PWM_RESOLUTION = 8;    // 8-bit: 0-255

// ===================== Globals =====================
NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pTxCharacteristic = nullptr;
bool deviceConnected = false;

int currentSpeed = 200;          // Default speed (0-255)
unsigned long lastCmdTime = 0;
const unsigned long CMD_TIMEOUT = 500; // Auto-stop if no command for 500ms

// ===================== Motor Control =====================
void initMotors() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // ESP32 Arduino Core 3.0+: ledcAttach binds pin to PWM
  ledcAttach(ENA, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(ENB, PWM_FREQ, PWM_RESOLUTION);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  ledcWrite(ENA, 0);
  ledcWrite(ENB, 0);
}

void setLeftMotor(bool forward, int speed) {
  digitalWrite(IN1, forward ? HIGH : LOW);
  digitalWrite(IN2, forward ? LOW : HIGH);
  ledcWrite(ENA, constrain(speed, 0, 255));
}

void setRightMotor(bool forward, int speed) {
  digitalWrite(IN3, forward ? HIGH : LOW);
  digitalWrite(IN4, forward ? LOW : HIGH);
  ledcWrite(ENB, constrain(speed, 0, 255));
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  ledcWrite(ENA, 0);
  ledcWrite(ENB, 0);
}

void moveForward(int speed) {
  setLeftMotor(true, speed);
  setRightMotor(true, speed);
}

void moveBackward(int speed) {
  setLeftMotor(false, speed);
  setRightMotor(false, speed);
}

void turnLeft(int speed) {
  // Pivot left: left motor backward, right motor forward
  setLeftMotor(false, speed);
  setRightMotor(true, speed);
}

void turnRight(int speed) {
  // Pivot right: right motor backward, left motor forward
  setLeftMotor(true, speed);
  setRightMotor(false, speed);
}

void spinLeft(int speed) {
  // Gentle left: left motor stop/slow, right motor forward
  setLeftMotor(true, speed / 3);
  setRightMotor(true, speed);
}

void spinRight(int speed) {
  // Gentle right: right motor stop/slow, left motor forward
  setLeftMotor(true, speed);
  setRightMotor(true, speed / 3);
}

// ===================== Command Parser =====================
void processCommand(const std::string& cmd) {
  if (cmd.empty()) return;

  char c = cmd[0];
  lastCmdTime = millis();

  // Speed setting: '0'-'9' maps to 0-255
  if (c >= '0' && c <= '9') {
    currentSpeed = map(c - '0', 0, 9, 0, 255);
    Serial.printf("Speed set to %d/255\n", currentSpeed);
    return;
  }

  // Advanced command with inline speed, e.g., "F180" or "L120"
  int explicitSpeed = currentSpeed;
  if (cmd.length() > 1) {
    int val = atoi(cmd.c_str() + 1);
    if (val > 0 && val <= 255) explicitSpeed = val;
  }

  switch (c) {
    case 'F': case 'f':
      moveForward(explicitSpeed);
      Serial.printf("Forward @ %d\n", explicitSpeed);
      break;
    case 'B': case 'b':
      moveBackward(explicitSpeed);
      Serial.printf("Backward @ %d\n", explicitSpeed);
      break;
    case 'L':
      turnLeft(explicitSpeed);   // Pivot turn (sharp)
      Serial.printf("Turn Left (pivot) @ %d\n", explicitSpeed);
      break;
    case 'R':
      turnRight(explicitSpeed);  // Pivot turn (sharp)
      Serial.printf("Turn Right (pivot) @ %d\n", explicitSpeed);
      break;
    case 'l':
      spinLeft(explicitSpeed);   // Gentle turn
      Serial.printf("Spin Left (gentle) @ %d\n", explicitSpeed);
      break;
    case 'r':
      spinRight(explicitSpeed);  // Gentle turn
      Serial.printf("Spin Right (gentle) @ %d\n", explicitSpeed);
      break;
    case 'S': case 's':
      stopMotors();
      Serial.println("Stop");
      break;
    default:
      break;
  }
}

// ===================== BLE Callbacks =====================
class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer) override {
    deviceConnected = true;
    Serial.println("BLE: Client connected");
    digitalWrite(LED_PIN, HIGH);
  }

  void onDisconnect(NimBLEServer* pServer) override {
    deviceConnected = false;
    Serial.println("BLE: Client disconnected");
    digitalWrite(LED_PIN, LOW);
    stopMotors();

    // Restart advertising so we can reconnect
    NimBLEDevice::startAdvertising();
  }
};

class RxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic) override {
    std::string value = pCharacteristic->getValue();
    if (!value.empty()) {
      processCommand(value);
    }
  }
};

// ===================== Setup & Loop =====================
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000); // Wait for Serial but not forever

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  initMotors();

  // Init BLE
  NimBLEDevice::init("ESP32-S3-Car");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9); // +9 dBm for max range

  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  NimBLEService* pService = pServer->createService(SERVICE_UUID);

  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        NIMBLE_PROPERTY::NOTIFY
                      );

  NimBLECharacteristic* pRxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_RX,
                        NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::WRITE
                      );
  pRxCharacteristic->setCallbacks(new RxCallbacks());

  pService->start();

  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections
  pAdvertising->setMaxPreferred(0x12);
  NimBLEDevice::startAdvertising();

  Serial.println("ESP32-S3 BLE Car ready. Advertising as 'ESP32-S3-Car'");
  Serial.println("Commands: F/B/L/R/l/r/S, 0-9 for speed, e.g., F200");
}

void loop() {
  // Safety: auto-stop if BLE disconnected or no recent command
  if (!deviceConnected || (millis() - lastCmdTime > CMD_TIMEOUT && lastCmdTime != 0)) {
    if (digitalRead(IN1) == HIGH || digitalRead(IN3) == HIGH) {
      stopMotors();
      Serial.println("Auto-stop (safety)");
    }
  }

  // Small delay to yield to BLE stack
  delay(20);
}
