#pragma once

const char WIFI_CONTROL_AP_SSID[] = "ESP32-S3-Car";
const char WIFI_CONTROL_AP_PASSWORD[] = "12345678";
const int WIFI_CONTROL_PORT = 80;
const int WIFI_CONTROL_DNS_PORT = 53;

inline bool wifiControlPasswordIsValid() {
  return sizeof(WIFI_CONTROL_AP_PASSWORD) - 1 >= 8;
}
