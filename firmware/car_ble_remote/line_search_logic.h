#pragma once

const int LINE_TRACK_MIN_SPEED = 80;
const int LINE_TRACK_MAX_SPEED = 210;
const int LINE_TRACK_CURVE_MAX_SPEED = 165;
const int LINE_TRACK_THICK_LINE_MAX_SPEED = 145;
const int LINE_TRACK_ERROR_SCALE = 10;
const int LINE_TRACK_KP = 42;
const int LINE_TRACK_KD = 24;

const int LINE_SEARCH_BACKTRACK_SPEED = 62;
const int LINE_SEARCH_BACKTRACK_BIAS = 24;
const int LINE_SEARCH_TURN_SPEED = 84;
const int LINE_SEARCH_TURN_STEP = 6;
const int LINE_SEARCH_MAX_TURN_SPEED = 122;
const int LINE_SEARCH_BACKTRACK_EVERY_STEPS = 4;

const unsigned long LINE_SEARCH_BACKTRACK_MS = 260;
const unsigned long LINE_SEARCH_STEP_MS = 220;
const unsigned long LINE_SEARCH_MAX_MS = 3200;
const unsigned long LINE_SEARCH_MEMORY_MS = 1200;

struct SearchMotorCommand {
  int left;
  int right;
};

inline int chooseLineSearchDirection(int lastLineError, long randomBit) {
  if (lastLineError < 0) return -1;
  if (lastLineError > 0) return 1;
  return randomBit == 0 ? -1 : 1;
}

inline bool lineSearchMemoryIsFresh(unsigned long nowMs, unsigned long lastSeenMs) {
  return lastSeenMs > 0 && nowMs - lastSeenMs <= LINE_SEARCH_MEMORY_MS;
}

inline int lineSearchRememberedError(
  unsigned long nowMs,
  unsigned long lastSeenMs,
  int lastSeenError,
  unsigned long lastDirectionalSeenMs,
  int lastDirectionalError,
  int fallbackError,
  int routeBias
) {
  if (routeBias != 0) return routeBias;
  if (lastDirectionalError != 0 && lineSearchMemoryIsFresh(nowMs, lastDirectionalSeenMs)) {
    return lastDirectionalError;
  }
  if (lastSeenError != 0 && lineSearchMemoryIsFresh(nowMs, lastSeenMs)) {
    return lastSeenError;
  }
  return fallbackError;
}

inline bool lineSearchDirectionIsLocked(int rememberedError) {
  return rememberedError != 0;
}

inline int lineSearchNextDirection(
  int currentDirection,
  bool directionLocked,
  long randomPick,
  long randomDirection
) {
  if (directionLocked) return currentDirection < 0 ? -1 : 1;
  if (randomPick == 0) return randomDirection == 0 ? -1 : 1;
  return currentDirection < 0 ? 1 : -1;
}

inline bool lineSearchShouldAdvanceStep(unsigned long elapsedMs, unsigned long stepElapsedMs) {
  return elapsedMs >= LINE_SEARCH_BACKTRACK_MS + LINE_SEARCH_STEP_MS
    && stepElapsedMs >= LINE_SEARCH_STEP_MS;
}

inline int clampLineSpeed(int value, int minValue, int maxValue) {
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}

inline int lineAbs(int value) {
  return value < 0 ? -value : value;
}

inline int lineTrackingActiveCount(unsigned int sensorMask, int sensorCount) {
  int activeCount = 0;
  for (int i = 0; i < sensorCount; i++) {
    if ((sensorMask & (1u << i)) != 0) activeCount++;
  }
  return activeCount;
}

inline int lineTrackingSignalSpan(unsigned int sensorMask, int sensorCount) {
  int first = -1;
  int last = -1;
  for (int i = 0; i < sensorCount; i++) {
    if ((sensorMask & (1u << i)) != 0) {
      if (first < 0) first = i;
      last = i;
    }
  }
  if (first < 0) return 0;
  return last - first + 1;
}

inline bool lineTrackingShouldSearchWide(int activeCount) {
  return activeCount >= 4;
}

inline bool lineTrackingShouldSearchWide(unsigned int sensorMask, int sensorCount) {
  int activeCount = lineTrackingActiveCount(sensorMask, sensorCount);
  if (lineTrackingShouldSearchWide(activeCount)) return true;
  return lineTrackingSignalSpan(sensorMask, sensorCount) > 3;
}

inline int lineTrackingPositionError(unsigned int sensorMask, int sensorCount, int lastError) {
  int weightedSum = 0;
  int activeCount = 0;
  int center = sensorCount / 2;

  for (int i = 0; i < sensorCount; i++) {
    if ((sensorMask & (1u << i)) != 0) {
      weightedSum += i - center;
      activeCount++;
    }
  }

  if (activeCount <= 0) return lastError;
  return weightedSum * LINE_TRACK_ERROR_SCALE / activeCount;
}

inline int lineTrackingBaseSpeed(int requestedSpeed, int activeCount, int error) {
  int base = clampLineSpeed(requestedSpeed, LINE_TRACK_MIN_SPEED, LINE_TRACK_MAX_SPEED);
  if (activeCount >= 3) {
    base = clampLineSpeed(base, LINE_TRACK_MIN_SPEED, LINE_TRACK_THICK_LINE_MAX_SPEED);
  }
  if (activeCount <= 2 || error != 0) {
    base = clampLineSpeed(base, LINE_TRACK_MIN_SPEED, LINE_TRACK_CURVE_MAX_SPEED);
  }
  return base;
}

inline int lineTrackingCorrection(int error, int derivative) {
  return (LINE_TRACK_KP * error + LINE_TRACK_KD * derivative) / LINE_TRACK_ERROR_SCALE;
}

inline SearchMotorCommand lineTrackingMotorCommand(int base, int error, int derivative, int activeCount) {
  if (activeCount >= 4) return {base, base};

  int correction = lineTrackingCorrection(error, derivative);
  int left = clampLineSpeed(base + correction, -255, 255);
  int right = clampLineSpeed(base - correction, -255, 255);
  return {left, right};
}

inline SearchMotorCommand lineWideSearchMotorCommand(int direction) {
  int dir = direction < 0 ? -1 : 1;
  return {dir * LINE_SEARCH_TURN_SPEED, -dir * LINE_SEARCH_TURN_SPEED};
}

inline SearchMotorCommand lineSearchBacktrackCommand(int direction) {
  int fastReverse = LINE_SEARCH_BACKTRACK_SPEED + LINE_SEARCH_BACKTRACK_BIAS;
  if (direction < 0) {
    return {-fastReverse, -LINE_SEARCH_BACKTRACK_SPEED};
  }
  return {-LINE_SEARCH_BACKTRACK_SPEED, -fastReverse};
}

inline SearchMotorCommand lineSearchMotorCommand(unsigned long elapsedMs, int direction, int step) {
  if (elapsedMs < LINE_SEARCH_BACKTRACK_MS) {
    return lineSearchBacktrackCommand(direction);
  }

  if (step > 0 && step % LINE_SEARCH_BACKTRACK_EVERY_STEPS == 0) {
    return {-LINE_SEARCH_BACKTRACK_SPEED, -LINE_SEARCH_BACKTRACK_SPEED};
  }

  int dir = direction < 0 ? -1 : 1;
  int turnSpeed = LINE_SEARCH_TURN_SPEED + step * LINE_SEARCH_TURN_STEP;
  if (turnSpeed > LINE_SEARCH_MAX_TURN_SPEED) turnSpeed = LINE_SEARCH_MAX_TURN_SPEED;
  return {dir * turnSpeed, -dir * turnSpeed};
}
