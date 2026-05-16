#include <cassert>

#include "../firmware/car_ble_remote/oled_boot_animation.h"

int main() {
  assert(bootAnimationStyle() == BootAnimationStyle::HudStartup);
  assert(BOOT_TRANSFORM_FRAME_COUNT == 8);
  assert(BOOT_TRANSFORM_FRAME_DELAY_MS >= 100);
  assert(BOOT_TRANSFORM_FRAME_DELAY_MS <= 150);

  const unsigned long duration = bootTransformDurationMs();
  assert(duration >= 800);
  assert(duration <= 1200);

  return 0;
}
