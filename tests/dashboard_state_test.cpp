#include <cassert>
#include <cstring>

#include "../firmware/car_ble_remote/dashboard_state.h"

int main() {
  assert(dashboardArrowForCommand('F') == '^');
  assert(dashboardArrowForCommand('B') == 'v');
  assert(dashboardArrowForCommand('L') == '<');
  assert(dashboardArrowForCommand('R') == '>');
  assert(dashboardArrowForCommand('l') == '<');
  assert(dashboardArrowForCommand('r') == '>');
  assert(dashboardArrowForCommand('S') == '-');
  assert(dashboardArrowForCommand('T') == '-');

  assert(dashboardModeLabel(false) == DashboardModeLabel::Remote);
  assert(dashboardModeLabel(true) == DashboardModeLabel::AutoLine);

  assert(dashboardShouldRefresh(0, 0));
  assert(!dashboardShouldRefresh(100, 1));
  assert(dashboardShouldRefresh(251, 1));
  assert(!dashboardHasControlClient(false, 0));
  assert(dashboardHasControlClient(true, 0));
  assert(dashboardHasControlClient(false, 1));
  assert(dashboardHasControlClient(true, 2));
  assert(dashboardStatusLedShouldBlink(false, 0));
  assert(!dashboardStatusLedShouldBlink(true, 0));
  assert(!dashboardStatusLedShouldBlink(false, 1));

  assert(dashboardLineSensorIndexForSlot(0, 5) == 4);
  assert(dashboardLineSensorIndexForSlot(2, 5) == 2);
  assert(dashboardLineSensorIndexForSlot(4, 5) == 0);
  assert(dashboardLineSensorSlotActive(0b10000, 0, 5));
  assert(!dashboardLineSensorSlotActive(0b00001, 0, 5));
  assert(dashboardLineSensorSlotActive(0b00100, 2, 5));
  assert(dashboardLineSensorSlotActive(0b00001, 4, 5));
  assert(dashboardLineQuality(0, 0) == DashboardLineQuality::Lost);
  assert(dashboardLineQuality(5, 0) == DashboardLineQuality::Wide);
  assert(dashboardLineQuality(1, -1) == DashboardLineQuality::Left);
  assert(dashboardLineQuality(1, -900) == DashboardLineQuality::Left);
  assert(dashboardLineQuality(1, 1) == DashboardLineQuality::Right);
  assert(dashboardLineQuality(1, 900) == DashboardLineQuality::Right);
  assert(dashboardLineQuality(3, 0) == DashboardLineQuality::Center);
  assert(std::strcmp(dashboardLineQualityCode(DashboardLineQuality::Lost), "LOST") == 0);
  assert(std::strcmp(dashboardLineQualityCode(DashboardLineQuality::Left), "LEFT") == 0);
  assert(std::strcmp(dashboardLineQualityCode(DashboardLineQuality::Center), "CENTER") == 0);
  assert(std::strcmp(dashboardLineQualityCode(DashboardLineQuality::Right), "RIGHT") == 0);
  assert(std::strcmp(dashboardLineQualityCode(DashboardLineQuality::Wide), "WIDE") == 0);

  DashboardSmileMouth smile = dashboardIdleSmileMouth();
  assert(smile.middleY > smile.cornerY);
  assert(smile.middleWidth >= 10);

  DashboardIdleFace face = dashboardIdleFace();
  assert(face.radius >= 14);
  assert(face.centerX == 64);
  assert(face.centerY >= 29);

  return 0;
}
