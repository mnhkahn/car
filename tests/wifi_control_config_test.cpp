#include <cassert>
#include <cstring>

#include "../firmware/car_ble_remote/wifi_control_config.h"

int main() {
  assert(std::strcmp(WIFI_CONTROL_AP_SSID, "ESP32-S3-Car") == 0);
  assert(std::strlen(WIFI_CONTROL_AP_PASSWORD) >= 8);
  assert(WIFI_CONTROL_PORT == 80);
  assert(WIFI_CONTROL_DNS_PORT == 53);
  assert(wifiControlPasswordIsValid());

  return 0;
}
