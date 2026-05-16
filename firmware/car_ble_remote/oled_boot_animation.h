#pragma once

enum class BootAnimationStyle {
  HudStartup
};

const int BOOT_TRANSFORM_FRAME_COUNT = 8;
const unsigned long BOOT_TRANSFORM_FRAME_DELAY_MS = 120;

inline BootAnimationStyle bootAnimationStyle() {
  return BootAnimationStyle::HudStartup;
}

inline unsigned long bootTransformDurationMs() {
  return BOOT_TRANSFORM_FRAME_COUNT * BOOT_TRANSFORM_FRAME_DELAY_MS;
}
