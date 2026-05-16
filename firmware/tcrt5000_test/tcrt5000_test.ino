// TCRT5000 five-way tracking sensor test for ESP32-S3.
// Wire sensor outputs from left to right to GPIO 13, 14, 15, 16, 17.

#define SENSOR_COUNT 5

const uint8_t SENSOR_PINS[SENSOR_COUNT] = {13, 14, 15, 16, 17};
const char *SENSOR_NAMES[SENSOR_COUNT] = {"L2", "L1", "C", "R1", "R2"};

// Most LM393/TCRT5000 tracking modules output LOW when seeing black.
// If your serial output is reversed, change this to HIGH.
const int BLACK_LEVEL = LOW;

void setup() {
  Serial.begin(115200);
  delay(1500);

  for (int i = 0; i < SENSOR_COUNT; i++) {
    pinMode(SENSOR_PINS[i], INPUT_PULLUP);
  }

  Serial.println();
  Serial.println("TCRT5000 5-way sensor test");
  Serial.println("Pins: L2=13 L1=14 C=15 R1=16 R2=17");
  Serial.println("raw: digitalRead value, black: detected black line");
}

void loop() {
  Serial.print("raw ");
  for (int i = 0; i < SENSOR_COUNT; i++) {
    int raw = digitalRead(SENSOR_PINS[i]);
    Serial.print(SENSOR_NAMES[i]);
    Serial.print('=');
    Serial.print(raw);
    Serial.print(' ');
  }

  Serial.print("| black ");
  bool anyBlack = false;
  for (int i = 0; i < SENSOR_COUNT; i++) {
    bool black = digitalRead(SENSOR_PINS[i]) == BLACK_LEVEL;
    Serial.print(SENSOR_NAMES[i]);
    Serial.print('=');
    Serial.print(black ? '1' : '0');
    Serial.print(' ');
    anyBlack = anyBlack || black;
  }

  Serial.print("| line=");
  Serial.println(anyBlack ? "YES" : "NO");
  delay(200);
}
