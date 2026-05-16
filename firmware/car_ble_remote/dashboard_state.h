#pragma once

const unsigned long DASHBOARD_REFRESH_MS = 250;

enum class DashboardModeLabel {
  Remote,
  AutoLine
};

enum class DashboardLineQuality {
  Lost,
  Left,
  Center,
  Right,
  Wide
};

struct DashboardSmileMouth {
  int leftX;
  int rightX;
  int cornerY;
  int middleY;
  int middleWidth;
};

struct DashboardIdleFace {
  int centerX;
  int centerY;
  int radius;
};

inline char dashboardArrowForCommand(char cmd) {
  switch (cmd) {
    case 'F': return '^';
    case 'B': return 'v';
    case 'L':
    case 'l': return '<';
    case 'R':
    case 'r': return '>';
    default: return '-';
  }
}

inline DashboardModeLabel dashboardModeLabel(bool lineTracking) {
  return lineTracking ? DashboardModeLabel::AutoLine : DashboardModeLabel::Remote;
}

inline bool dashboardShouldRefresh(unsigned long now, unsigned long lastRefresh) {
  return lastRefresh == 0 || now - lastRefresh >= DASHBOARD_REFRESH_MS;
}

inline bool dashboardHasControlClient(bool bleConnected, int wifiStationCount) {
  return bleConnected || wifiStationCount > 0;
}

inline bool dashboardStatusLedShouldBlink(bool bleConnected, int wifiStationCount) {
  return !dashboardHasControlClient(bleConnected, wifiStationCount);
}

inline int dashboardLineSensorIndexForSlot(int slot, int sensorCount) {
  return sensorCount - 1 - slot;
}

inline bool dashboardLineSensorSlotActive(unsigned int sensorMask, int slot, int sensorCount) {
  int sensorIndex = dashboardLineSensorIndexForSlot(slot, sensorCount);
  return (sensorMask & (1u << sensorIndex)) != 0;
}

inline DashboardLineQuality dashboardLineQuality(int activeCount, int error) {
  if (activeCount <= 0) return DashboardLineQuality::Lost;
  if (activeCount >= 4) return DashboardLineQuality::Wide;
  if (error < 0) return DashboardLineQuality::Left;
  if (error > 0) return DashboardLineQuality::Right;
  return DashboardLineQuality::Center;
}

inline const char *dashboardLineQualityCode(DashboardLineQuality quality) {
  switch (quality) {
    case DashboardLineQuality::Lost:
      return "LOST";
    case DashboardLineQuality::Left:
      return "LEFT";
    case DashboardLineQuality::Center:
      return "CENTER";
    case DashboardLineQuality::Right:
      return "RIGHT";
    case DashboardLineQuality::Wide:
      return "WIDE";
    default:
      return "LOST";
  }
}

inline DashboardSmileMouth dashboardIdleSmileMouth() {
  return {54, 74, 32, 36, 12};
}

inline DashboardIdleFace dashboardIdleFace() {
  return {64, 30, 15};
}
