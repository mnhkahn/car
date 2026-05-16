#include <cassert>

#include "../firmware/car_ble_remote/line_search_logic.h"

int main() {
  assert(chooseLineSearchDirection(-2, 1) == -1);
  assert(chooseLineSearchDirection(2, 0) == 1);
  assert(chooseLineSearchDirection(0, 0) == -1);
  assert(chooseLineSearchDirection(0, 1) == 1);
  assert(lineSearchMemoryIsFresh(2000, 2000 - LINE_SEARCH_MEMORY_MS));
  assert(!lineSearchMemoryIsFresh(2000, 2000 - LINE_SEARCH_MEMORY_MS - 1));
  assert(lineSearchRememberedError(3000, 2800, 0, 2700, -10, 0, 1) == 1);
  assert(lineSearchRememberedError(3000, 2900, 0, 2850, -10, 0, 0) == -10);
  assert(lineSearchRememberedError(3000, 2900, 10, 1000, -10, 0, 0) == 10);
  assert(lineSearchRememberedError(3000, 1000, 10, 1000, -10, -20, 0) == -20);
  assert(lineSearchDirectionIsLocked(-10));
  assert(lineSearchDirectionIsLocked(10));
  assert(!lineSearchDirectionIsLocked(0));
  assert(lineSearchNextDirection(-1, true, 0, 1) == -1);
  assert(lineSearchNextDirection(1, true, 0, 0) == 1);
  assert(lineSearchNextDirection(1, false, 1, 0) == -1);
  assert(lineSearchNextDirection(1, false, 0, 0) == -1);
  assert(lineSearchNextDirection(1, false, 0, 1) == 1);
  assert(!lineSearchShouldAdvanceStep(LINE_SEARCH_BACKTRACK_MS - 1, LINE_SEARCH_STEP_MS + 10));
  assert(!lineSearchShouldAdvanceStep(
    LINE_SEARCH_BACKTRACK_MS + LINE_SEARCH_STEP_MS - 1,
    LINE_SEARCH_STEP_MS + 10
  ));
  assert(lineSearchShouldAdvanceStep(
    LINE_SEARCH_BACKTRACK_MS + LINE_SEARCH_STEP_MS,
    LINE_SEARCH_STEP_MS
  ));

  SearchMotorCommand leftBacktrack = lineSearchMotorCommand(0, -1, 0);
  assert(leftBacktrack.left < 0);
  assert(leftBacktrack.right < 0);
  assert(leftBacktrack.left < leftBacktrack.right);

  SearchMotorCommand rightBacktrack = lineSearchMotorCommand(0, 1, 0);
  assert(rightBacktrack.left < 0);
  assert(rightBacktrack.right < 0);
  assert(rightBacktrack.right < rightBacktrack.left);

  SearchMotorCommand leftTurn = lineSearchMotorCommand(LINE_SEARCH_BACKTRACK_MS + 1, -1, 0);
  assert(leftTurn.left < 0);
  assert(leftTurn.right > 0);

  SearchMotorCommand rightTurn = lineSearchMotorCommand(LINE_SEARCH_BACKTRACK_MS + 1, 1, 0);
  assert(rightTurn.left > 0);
  assert(rightTurn.right < 0);

  SearchMotorCommand wideLeft = lineWideSearchMotorCommand(-1);
  assert(wideLeft.left < 0);
  assert(wideLeft.right > 0);

  SearchMotorCommand wideRight = lineWideSearchMotorCommand(1);
  assert(wideRight.left > 0);
  assert(wideRight.right < 0);

  SearchMotorCommand retryBacktrack = lineSearchMotorCommand(
    LINE_SEARCH_BACKTRACK_MS + 1,
    1,
    LINE_SEARCH_BACKTRACK_EVERY_STEPS
  );
  assert(retryBacktrack.left < 0);
  assert(retryBacktrack.right < 0);

  SearchMotorCommand lateSearch = lineSearchMotorCommand(LINE_SEARCH_MAX_MS + 1, 1, 20);
  assert(lateSearch.left != 0 || lateSearch.right != 0);

  assert(lineTrackingBaseSpeed(200, 1, 20) == LINE_TRACK_CURVE_MAX_SPEED);
  assert(lineTrackingBaseSpeed(200, 3, 0) == LINE_TRACK_THICK_LINE_MAX_SPEED);
  assert(lineTrackingBaseSpeed(255, 3, 0) == LINE_TRACK_THICK_LINE_MAX_SPEED);
  assert(lineTrackingBaseSpeed(30, 5, 0) == LINE_TRACK_MIN_SPEED);

  assert(lineTrackingPositionError(0b00100, 5, 0) == 0);
  assert(lineTrackingPositionError(0b00110, 5, 0) == -5);
  assert(lineTrackingPositionError(0b01100, 5, 0) == 5);
  assert(lineTrackingPositionError(0b00001, 5, 0) == -20);
  assert(lineTrackingPositionError(0b10000, 5, 0) == 20);
  assert(lineTrackingPositionError(0, 5, -10) == -10);
  assert(lineTrackingActiveCount(0b10101, 5) == 3);
  assert(lineTrackingSignalSpan(0b00000, 5) == 0);
  assert(lineTrackingSignalSpan(0b00100, 5) == 1);
  assert(lineTrackingSignalSpan(0b00110, 5) == 2);
  assert(lineTrackingSignalSpan(0b00111, 5) == 3);
  assert(lineTrackingSignalSpan(0b10101, 5) == 5);
  assert(lineTrackingBaseSpeed(200, 2, 10) == LINE_TRACK_CURVE_MAX_SPEED);
  assert(lineTrackingBaseSpeed(200, 5, 0) == LINE_TRACK_THICK_LINE_MAX_SPEED);

  SearchMotorCommand centered = lineTrackingMotorCommand(150, 0, 0, 1);
  assert(centered.left == 150);
  assert(centered.right == 150);

  assert(lineTrackingShouldSearchWide(3) == false);
  assert(lineTrackingShouldSearchWide(4));
  assert(lineTrackingShouldSearchWide(5));
  assert(!lineTrackingShouldSearchWide(0b00111, 5));
  assert(!lineTrackingShouldSearchWide(0b01110, 5));
  assert(!lineTrackingShouldSearchWide(0b11100, 5));
  assert(lineTrackingShouldSearchWide(0b01111, 5));
  assert(lineTrackingShouldSearchWide(0b10101, 5));
  assert(lineTrackingShouldSearchWide(0b10001, 5));

  SearchMotorCommand leftCorrection = lineTrackingMotorCommand(150, -10, 0, 1);
  assert(leftCorrection.left < leftCorrection.right);
  assert(leftCorrection.left >= 0);

  SearchMotorCommand rightCorrection = lineTrackingMotorCommand(150, 10, 0, 1);
  assert(rightCorrection.right < rightCorrection.left);
  assert(rightCorrection.right >= 0);

  return 0;
}
