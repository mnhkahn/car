#include <cassert>

#include "../firmware/car_ble_remote/manual_drive_logic.h"

int main() {
  DriveMotorCommand forward = manualDriveMotorCommand('F', 180);
  assert(forward.left == 180);
  assert(forward.right == 180);

  DriveMotorCommand backward = manualDriveMotorCommand('B', 180);
  assert(backward.left == -180);
  assert(backward.right == -180);

  DriveMotorCommand left = manualDriveMotorCommand('L', 180);
  assert(left.left == -180);
  assert(left.right == 180);

  DriveMotorCommand right = manualDriveMotorCommand('R', 180);
  assert(right.left == 180);
  assert(right.right == -180);

  DriveMotorCommand softLeft = manualDriveMotorCommand('l', 180);
  assert(softLeft.left > 0);
  assert(softLeft.right > 0);
  assert(softLeft.left < softLeft.right);

  DriveMotorCommand softRight = manualDriveMotorCommand('r', 180);
  assert(softRight.left > 0);
  assert(softRight.right > 0);
  assert(softRight.right < softRight.left);

  DriveMotorCommand stop = manualDriveMotorCommand('S', 180);
  assert(stop.left == 0);
  assert(stop.right == 0);

  return 0;
}
