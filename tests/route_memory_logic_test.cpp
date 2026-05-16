#include <cassert>

#include "../firmware/car_ble_remote/route_memory_logic.h"

int main() {
  assert(routeKindForLine(0, 0) == RouteSegmentKind::Lost);
  assert(routeKindForLine(5, 0) == RouteSegmentKind::Wide);
  assert(routeKindForLine(3, 0) == RouteSegmentKind::Straight);
  assert(routeKindForLine(2, -1) == RouteSegmentKind::Left);
  assert(routeKindForLine(2, 1) == RouteSegmentKind::Right);

  RouteEvent events[ROUTE_MAX_EVENTS] = {};
  int count = 0;
  assert(routeRecordSample(events, count, RouteSegmentKind::Straight));
  assert(count == 1);
  assert(events[0].kind == RouteSegmentKind::Straight);
  assert(events[0].durationTicks == 1);

  assert(routeRecordSample(events, count, RouteSegmentKind::Straight));
  assert(count == 1);
  assert(events[0].durationTicks == 2);

  assert(routeRecordSample(events, count, RouteSegmentKind::Left));
  assert(count == 2);
  assert(events[1].kind == RouteSegmentKind::Left);
  assert(events[1].durationTicks == 1);

  assert(routeTotalTicks(events, count) == 3);
  assert(routeEventIndexForTick(events, count, 0) == 0);
  assert(routeEventIndexForTick(events, count, 1) == 0);
  assert(routeEventIndexForTick(events, count, 2) == 1);
  assert(routeEventIndexForTick(events, count, 3) == 0);

  assert(routeKindMatches(RouteSegmentKind::Straight, RouteSegmentKind::Straight));
  assert(!routeKindMatches(RouteSegmentKind::Left, RouteSegmentKind::Right));

  assert(routeSpeedLimitForMode(RouteRunMode::Learn, RouteSegmentKind::Straight, false) == ROUTE_LEARN_SPEED);
  assert(routeSpeedLimitForMode(RouteRunMode::Replay, RouteSegmentKind::Straight, false) == ROUTE_REPLAY_STRAIGHT_SPEED);
  assert(routeSpeedLimitForMode(RouteRunMode::Replay, RouteSegmentKind::Left, false) == ROUTE_REPLAY_CURVE_SPEED);
  assert(routeSpeedLimitForMode(RouteRunMode::Replay, RouteSegmentKind::Wide, false) == ROUTE_REPLAY_WIDE_SPEED);
  assert(routeSpeedLimitForMode(RouteRunMode::Replay, RouteSegmentKind::Lost, false) == ROUTE_REPLAY_LOST_SPEED);
  assert(routeSpeedLimitForMode(RouteRunMode::Replay, RouteSegmentKind::Straight, true) == ROUTE_REPLAY_MISMATCH_SPEED);
  assert(ROUTE_LEARN_SPEED < ROUTE_REPLAY_STRAIGHT_SPEED);
  assert(ROUTE_REPLAY_CURVE_SPEED < ROUTE_REPLAY_STRAIGHT_SPEED);

  assert(routeKindCode(RouteSegmentKind::Straight) == 'S');
  assert(routeKindCode(RouteSegmentKind::Left) == 'L');
  assert(routeKindCode(RouteSegmentKind::Right) == 'R');
  assert(routeKindCode(RouteSegmentKind::Wide) == 'W');
  assert(routeKindCode(RouteSegmentKind::Lost) == 'X');
  assert(routeSearchErrorBias(RouteSegmentKind::Left) < 0);
  assert(routeSearchErrorBias(RouteSegmentKind::Right) > 0);
  assert(routeSearchErrorBias(RouteSegmentKind::Straight) == 0);
  assert(routeSearchErrorBias(RouteSegmentKind::Wide) == 0);
  assert(routeSearchErrorBias(RouteSegmentKind::Lost) == 0);

  RouteEvent mapEvents[] = {
    {RouteSegmentKind::Straight, 2},
    {RouteSegmentKind::Left, 1},
    {RouteSegmentKind::Straight, 1},
    {RouteSegmentKind::Right, 1},
    {RouteSegmentKind::Wide, 1},
    {RouteSegmentKind::Lost, 1},
  };
  RouteMapPoint points[8] = {};
  int pointCount = routeBuildMapPoints(mapEvents, 6, points, 8);
  assert(pointCount == 6);
  assert(points[0].x == 0 && points[0].y == -1 && points[0].kind == RouteSegmentKind::Straight);
  assert(points[1].x == -1 && points[1].y == -1 && points[1].kind == RouteSegmentKind::Left);
  assert(points[2].x == -2 && points[2].y == -1 && points[2].kind == RouteSegmentKind::Straight);
  assert(points[3].x == -2 && points[3].y == -2 && points[3].kind == RouteSegmentKind::Right);
  assert(points[4].x == -2 && points[4].y == -3 && points[4].kind == RouteSegmentKind::Wide);
  assert(points[5].x == -2 && points[5].y == -3 && points[5].kind == RouteSegmentKind::Lost);

  return 0;
}
