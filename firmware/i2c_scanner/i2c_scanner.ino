#include <Wire.h>

void setup() {
  Serial.begin(115200);
  delay(3000);
  Wire.begin(11, 12);
  Serial.println("Scanning I2C...");

  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found device at 0x");
      Serial.println(addr, HEX);
    }
  }
  Serial.println("Done.");
}

void loop() {}
