#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLECharacteristic.h>
#include <BLE2902.h>

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define ENA 5
#define IN1 6
#define IN2 7
#define ENB 8
#define IN3 9
#define IN4 10

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
BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
unsigned long lastCmdTime = 0;
unsigned long ledFlashTime = 0;
int speed = 200;
bool deviceConnected = false;
bool oldDeviceConnected = false;

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
      ledFlashTime = millis();
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
  pinMode(LED_BUILTIN, OUTPUT);
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

  // LED status
  if (!deviceConnected) {
    digitalWrite(LED_BUILTIN, (millis() / 500) % 2);
  } else if (millis() - ledFlashTime < 100) {
    digitalWrite(LED_BUILTIN, LOW);
  } else {
    digitalWrite(LED_BUILTIN, HIGH);
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
