#pragma once

#include <stdint.h>

const int ROUTE_MAX_EVENTS = 96;
const unsigned long ROUTE_RECORD_SAMPLE_MS = 120;

const int ROUTE_LEARN_SPEED = 90;
const int ROUTE_REPLAY_STRAIGHT_SPEED = 185;
const int ROUTE_REPLAY_CURVE_SPEED = 125;
const int ROUTE_REPLAY_WIDE_SPEED = 105;
const int ROUTE_REPLAY_LOST_SPEED = 85;
const int ROUTE_REPLAY_MISMATCH_SPEED = 110;
const uint32_t ROUTE_REPLAY_LOOKAHEAD_TICKS = 2;

enum class RouteSegmentKind : uint8_t {
  Straight = 0,
  Left = 1,
  Right = 2,
  Wide = 3,
  Lost = 4
};

enum class RouteRunMode : uint8_t {
  Off = 0,
  Learn = 1,
  Replay = 2
};

struct RouteEvent {
  RouteSegmentKind kind;
  uint16_t durationTicks;
};

enum class RouteMapHeading : uint8_t {
  North = 0,
  East = 1,
  South = 2,
  West = 3
};

struct RouteMapPoint {
  int x;
  int y;
  RouteSegmentKind kind;
  RouteMapHeading heading;
};

inline RouteSegmentKind routeKindForLine(int activeCount, int error) {
  if (activeCount <= 0) return RouteSegmentKind::Lost;
  if (activeCount >= 4) return RouteSegmentKind::Wide;
  if (error < 0) return RouteSegmentKind::Left;
  if (error > 0) return RouteSegmentKind::Right;
  return RouteSegmentKind::Straight;
}

inline char routeKindCode(RouteSegmentKind kind) {
  switch (kind) {
    case RouteSegmentKind::Straight:
      return 'S';
    case RouteSegmentKind::Left:
      return 'L';
    case RouteSegmentKind::Right:
      return 'R';
    case RouteSegmentKind::Wide:
      return 'W';
    case RouteSegmentKind::Lost:
    default:
      return 'X';
  }
}

inline bool routeRecordSample(RouteEvent *events, int &count, RouteSegmentKind kind) {
  if (count > 0 && events[count - 1].kind == kind) {
    if (events[count - 1].durationTicks < 65535) {
      events[count - 1].durationTicks++;
    }
    return true;
  }

  if (count >= ROUTE_MAX_EVENTS) return false;

  events[count].kind = kind;
  events[count].durationTicks = 1;
  count++;
  return true;
}

inline RouteMapHeading routeTurnLeft(RouteMapHeading heading) {
  return static_cast<RouteMapHeading>((static_cast<int>(heading) + 3) % 4);
}

inline RouteMapHeading routeTurnRight(RouteMapHeading heading) {
  return static_cast<RouteMapHeading>((static_cast<int>(heading) + 1) % 4);
}

inline void routeAdvancePoint(int &x, int &y, RouteMapHeading heading) {
  switch (heading) {
    case RouteMapHeading::North:
      y--;
      break;
    case RouteMapHeading::East:
      x++;
      break;
    case RouteMapHeading::South:
      y++;
      break;
    case RouteMapHeading::West:
      x--;
      break;
  }
}

inline int routeBuildMapPoints(const RouteEvent *events, int count, RouteMapPoint *points, int maxPoints) {
  if (count <= 0 || maxPoints <= 0) return 0;

  int x = 0;
  int y = 0;
  RouteMapHeading heading = RouteMapHeading::North;
  int pointCount = 0;

  for (int i = 0; i < count && pointCount < maxPoints; i++) {
    RouteSegmentKind kind = events[i].kind;
    if (kind == RouteSegmentKind::Left) {
      heading = routeTurnLeft(heading);
      routeAdvancePoint(x, y, heading);
    } else if (kind == RouteSegmentKind::Right) {
      heading = routeTurnRight(heading);
      routeAdvancePoint(x, y, heading);
    } else if (kind == RouteSegmentKind::Straight || kind == RouteSegmentKind::Wide) {
      routeAdvancePoint(x, y, heading);
    }

    points[pointCount] = {x, y, kind, heading};
    pointCount++;
  }

  return pointCount;
}

inline uint32_t routeTotalTicks(const RouteEvent *events, int count) {
  uint32_t total = 0;
  for (int i = 0; i < count; i++) {
    total += events[i].durationTicks;
  }
  return total;
}

inline int routeEventIndexForTick(const RouteEvent *events, int count, uint32_t tick) {
  if (count <= 0) return -1;

  uint32_t total = routeTotalTicks(events, count);
  if (total == 0) return 0;
  tick %= total;

  uint32_t cursor = 0;
  for (int i = 0; i < count; i++) {
    cursor += events[i].durationTicks;
    if (tick < cursor) return i;
  }
  return count - 1;
}

inline bool routeKindMatches(RouteSegmentKind expected, RouteSegmentKind observed) {
  return expected == observed;
}

inline int routeSearchErrorBias(RouteSegmentKind kind) {
  switch (kind) {
    case RouteSegmentKind::Left:
      return -1;
    case RouteSegmentKind::Right:
      return 1;
    case RouteSegmentKind::Straight:
    case RouteSegmentKind::Wide:
    case RouteSegmentKind::Lost:
    default:
      return 0;
  }
}

inline int routeSpeedLimitForMode(RouteRunMode mode, RouteSegmentKind expected, bool mismatch) {
  if (mode == RouteRunMode::Learn) return ROUTE_LEARN_SPEED;
  if (mode != RouteRunMode::Replay) return ROUTE_REPLAY_MISMATCH_SPEED;
  if (mismatch) return ROUTE_REPLAY_MISMATCH_SPEED;

  switch (expected) {
    case RouteSegmentKind::Straight:
      return ROUTE_REPLAY_STRAIGHT_SPEED;
    case RouteSegmentKind::Left:
    case RouteSegmentKind::Right:
      return ROUTE_REPLAY_CURVE_SPEED;
    case RouteSegmentKind::Wide:
      return ROUTE_REPLAY_WIDE_SPEED;
    case RouteSegmentKind::Lost:
    default:
      return ROUTE_REPLAY_LOST_SPEED;
  }
}
