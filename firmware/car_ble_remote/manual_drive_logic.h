#pragma once

const int MANUAL_SOFT_TURN_PERCENT = 55;

struct DriveMotorCommand {
  int left;
  int right;
};

inline int manualScaleSpeed(int speed, int percent) {
  return speed * percent / 100;
}

inline DriveMotorCommand manualDriveMotorCommand(char cmd, int speed) {
  int softSpeed = manualScaleSpeed(speed, MANUAL_SOFT_TURN_PERCENT);
  switch (cmd) {
    case 'F':
      return {speed, speed};
    case 'B':
      return {-speed, -speed};
    case 'L':
      return {-speed, speed};
    case 'R':
      return {speed, -speed};
    case 'l':
      return {softSpeed, speed};
    case 'r':
      return {speed, softSpeed};
    default:
      return {0, 0};
  }
}
