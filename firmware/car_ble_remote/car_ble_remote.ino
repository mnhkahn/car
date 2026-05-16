#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLECharacteristic.h>
#include <BLE2902.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "line_search_logic.h"
#include "manual_drive_logic.h"
#include "route_memory_logic.h"
#include "wifi_control_config.h"
#include "wifi_control_page.h"
#include "oled_boot_animation.h"
#include "dashboard_state.h"

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define ENA 5
#define IN1 6
#define IN2 7
#define ENB 8
#define IN3 9
#define IN4 10

// Onboard RGB LED (WS2812)
#define NEOPIXEL_PIN 48

// OLED SH1106
#define OLED_SDA 11
#define OLED_SCL 12
Adafruit_SH1106G display(128, 64, &Wire, -1);
DNSServer dnsServer;
WebServer webServer(WIFI_CONTROL_PORT);

// Five-way TCRT5000 IR tracking sensor.
// Order: far left, left, center, right, far right.
#define IR_SENSOR_COUNT 5
const uint8_t IR_PINS[IR_SENSOR_COUNT] = {13, 14, 15, 16, 17};
const int LINE_BLACK_LEVEL = LOW;

class MotorDriver {
public:
  void init() {
    ledcAttach(ENA, 1000, 8);
    ledcAttach(ENB, 1000, 8);
    pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
    stop();
  }
  void set(int left, int right) {
    digitalWrite(IN1, left > 0); digitalWrite(IN2, left < 0);
    digitalWrite(IN3, right > 0); digitalWrite(IN4, right < 0);
    ledcWrite(ENA, constrain(abs(left), 0, 255));
    ledcWrite(ENB, constrain(abs(right), 0, 255));
  }
  void stop() {
    digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
    ledcWrite(ENA, 0); ledcWrite(ENB, 0);
  }
};

MotorDriver motor;
Adafruit_NeoPixel rgb(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

enum DriveMode {
  MODE_REMOTE,
  MODE_LINE_TRACK,
  MODE_ROUTE_LEARN,
  MODE_ROUTE_REPLAY
};

Preferences routePrefs;
RouteEvent routeEvents[ROUTE_MAX_EVENTS];
int routeEventCount = 0;
bool routeRecordOverflow = false;
bool routeReplayMismatch = false;
unsigned long routeLastSampleAt = 0;
unsigned long routeReplayStartedAt = 0;
RouteSegmentKind routeExpectedKind = RouteSegmentKind::Straight;
RouteSegmentKind routeObservedKind = RouteSegmentKind::Lost;

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
unsigned long lastCmdTime = 0;
int speed = 200;
bool deviceConnected = false;
bool oldDeviceConnected = false;
DriveMode driveMode = MODE_REMOTE;
int lastLineError = 0;
int lastSeenLineError = 0;
int lastSeenLineActiveCount = 0;
unsigned long lastLineSeenAt = 0;
int lastDirectionalLineError = 0;
unsigned long lastDirectionalLineSeenAt = 0;
uint8_t lineSensorMask = 0;
bool lineSearchActive = false;
unsigned long lineSearchStartedAt = 0;
unsigned long lineSearchStepStartedAt = 0;
int lineSearchDirection = 1;
int lineSearchStep = 0;
bool lineSearchDirectionLocked = false;

// Current command for OLED display
char currentCmd = 'S';
char prevDisplayCmd = 0;
unsigned long lastDashboardRefresh = 0;

// LED state
unsigned long ledColorTime = 0;
uint32_t currentLedColor = 0;
const unsigned long LED_DURATION = 300;

uint32_t colorForCmd(char cmd) {
  switch (cmd) {
    case 'F': return Adafruit_NeoPixel::Color(0, 40, 0);
    case 'B': return Adafruit_NeoPixel::Color(40, 0, 0);
    case 'L': return Adafruit_NeoPixel::Color(0, 0, 40);
    case 'R': return Adafruit_NeoPixel::Color(40, 20, 0);
    case 'l': return Adafruit_NeoPixel::Color(0, 20, 20);
    case 'r': return Adafruit_NeoPixel::Color(20, 10, 0);
    case 'T': return Adafruit_NeoPixel::Color(0, 35, 10);
    case 'E': return Adafruit_NeoPixel::Color(25, 25, 0);
    case 'P': return Adafruit_NeoPixel::Color(0, 20, 35);
    case 'C': return Adafruit_NeoPixel::Color(35, 0, 20);
    case 'M': return Adafruit_NeoPixel::Color(15, 15, 15);
    case 'S': return Adafruit_NeoPixel::Color(0, 0, 0);
    default: return Adafruit_NeoPixel::Color(20, 20, 20);
  }
}

bool isAutoDriveMode() {
  return driveMode == MODE_LINE_TRACK
    || driveMode == MODE_ROUTE_LEARN
    || driveMode == MODE_ROUTE_REPLAY;
}

const char *driveModeText() {
  switch (driveMode) {
    case MODE_LINE_TRACK:
      return "AUTO";
    case MODE_ROUTE_LEARN:
      return "LEARN";
    case MODE_ROUTE_REPLAY:
      return "MAP";
    case MODE_REMOTE:
    default:
      return "REMOTE";
  }
}

const char *routeStatusText() {
  if (driveMode == MODE_ROUTE_LEARN) {
    return routeRecordOverflow ? "FULL" : "REC";
  }
  if (driveMode == MODE_ROUTE_REPLAY) {
    if (lineSearchActive) return "SEARCH";
    return routeReplayMismatch ? "SAFE" : "RUN";
  }
  if (driveMode == MODE_LINE_TRACK) {
    return lineSearchActive ? "SEARCH" : "TRACK";
  }
  return "";
}

void initIrSensors() {
  for (int i = 0; i < IR_SENSOR_COUNT; i++) {
    pinMode(IR_PINS[i], INPUT_PULLUP);
  }
}

bool lineSeesBlack(int index) {
  return (lineSensorMask & (1u << index)) != 0;
}

uint8_t readLineSensorMask() {
  uint8_t mask = 0;
  for (int i = 0; i < IR_SENSOR_COUNT; i++) {
    if (digitalRead(IR_PINS[i]) == LINE_BLACK_LEVEL) {
      mask |= 1u << i;
    }
  }
  return mask;
}

void updateLineSensorMask() {
  lineSensorMask = readLineSensorMask();
}

int wifiStationCount() {
  return WiFi.softAPgetStationNum();
}

bool hasControlClient() {
  return dashboardHasControlClient(deviceConnected, wifiStationCount());
}

void resetLineSearch() {
  lineSearchActive = false;
  lineSearchStartedAt = 0;
  lineSearchStepStartedAt = 0;
  lineSearchDirection = 1;
  lineSearchStep = 0;
  lineSearchDirectionLocked = false;
}

void startLineSearch() {
  unsigned long now = millis();
  lineSearchActive = true;
  lineSearchStartedAt = now;
  lineSearchStepStartedAt = now;
  int routeBias = 0;
  if (driveMode == MODE_ROUTE_REPLAY) {
    routeBias = routeSearchErrorBias(routeExpectedKind);
  }
  int rememberedError = lineSearchRememberedError(
    now,
    lastLineSeenAt,
    lastSeenLineError,
    lastDirectionalLineSeenAt,
    lastDirectionalLineError,
    lastLineError,
    routeBias
  );
  lineSearchDirection = chooseLineSearchDirection(rememberedError, random(0, 2));
  lineSearchDirectionLocked = lineSearchDirectionIsLocked(rememberedError);
  lineSearchStep = 0;
}

void runLineSearch() {
  if (!lineSearchActive) startLineSearch();

  unsigned long now = millis();
  if (lineSearchShouldAdvanceStep(now - lineSearchStartedAt, now - lineSearchStepStartedAt)) {
    lineSearchStepStartedAt = now;
    lineSearchStep++;
    lineSearchDirection = lineSearchNextDirection(
      lineSearchDirection,
      lineSearchDirectionLocked,
      random(0, 5),
      random(0, 2)
    );
  }

  SearchMotorCommand command = lineSearchMotorCommand(
    now - lineSearchStartedAt,
    lineSearchDirection,
    lineSearchStep
  );
  motor.set(command.left, command.right);
}

void runWideLineSearch() {
  if (!lineSearchActive) startLineSearch();

  unsigned long now = millis();
  unsigned long elapsedAsSearch = LINE_SEARCH_BACKTRACK_MS + (now - lineSearchStartedAt);
  if (lineSearchShouldAdvanceStep(elapsedAsSearch, now - lineSearchStepStartedAt)) {
    lineSearchStepStartedAt = now;
    lineSearchStep++;
    if (!lineSearchDirectionLocked) {
      lineSearchDirection = -lineSearchDirection;
    }
  }

  SearchMotorCommand command = lineWideSearchMotorCommand(lineSearchDirection);
  motor.set(command.left, command.right);
}

void applySpeedSuffix(const String &rxValue) {
  if (rxValue.length() <= 1) return;
  for (int i = 1; i < rxValue.length(); i++) {
    if (rxValue[i] < '0' || rxValue[i] > '9') return;
  }
  speed = constrain(rxValue.substring(1).toInt(), 0, 255);
}

void loadRouteMemory() {
  routePrefs.begin("carroute", true);
  int savedCount = routePrefs.getInt("count", 0);
  size_t storedBytes = routePrefs.getBytesLength("events");
  routePrefs.end();

  if (savedCount <= 0 || savedCount > ROUTE_MAX_EVENTS) {
    routeEventCount = 0;
    return;
  }

  size_t expectedBytes = sizeof(RouteEvent) * savedCount;
  if (storedBytes != expectedBytes) {
    routeEventCount = 0;
    return;
  }

  routePrefs.begin("carroute", true);
  size_t readBytes = routePrefs.getBytes("events", routeEvents, expectedBytes);
  routePrefs.end();
  routeEventCount = readBytes == expectedBytes ? savedCount : 0;
}

void saveRouteMemory() {
  routePrefs.begin("carroute", false);
  routePrefs.putInt("count", routeEventCount);
  routePrefs.putBytes("events", routeEvents, sizeof(RouteEvent) * routeEventCount);
  routePrefs.end();
}

void clearRouteMemory() {
  routeEventCount = 0;
  routeRecordOverflow = false;
  routeReplayMismatch = false;
  routeLastSampleAt = 0;
  routeReplayStartedAt = 0;
  routePrefs.begin("carroute", false);
  routePrefs.clear();
  routePrefs.end();
}

void setDriveMode(DriveMode mode) {
  driveMode = mode;
  lastLineError = 0;
  lastSeenLineError = 0;
  lastSeenLineActiveCount = 0;
  lastLineSeenAt = 0;
  lastDirectionalLineError = 0;
  lastDirectionalLineSeenAt = 0;
  resetLineSearch();
  routeReplayMismatch = false;

  switch (mode) {
    case MODE_LINE_TRACK:
      currentCmd = 'T';
      break;
    case MODE_ROUTE_LEARN:
      currentCmd = 'E';
      break;
    case MODE_ROUTE_REPLAY:
      currentCmd = 'P';
      break;
    default:
      currentCmd = 'S';
      motor.stop();
      break;
  }
}

void startRouteLearning() {
  routeEventCount = 0;
  routeRecordOverflow = false;
  routeReplayMismatch = false;
  routeLastSampleAt = millis();
  routeExpectedKind = RouteSegmentKind::Straight;
  routeObservedKind = RouteSegmentKind::Lost;
  setDriveMode(MODE_ROUTE_LEARN);
}

void finishRouteLearning() {
  if (routeEventCount > 0) {
    saveRouteMemory();
  }
}

void startRouteReplay() {
  if (routeEventCount <= 0) {
    startRouteLearning();
    return;
  }

  routeReplayStartedAt = millis();
  routeReplayMismatch = false;
  routeExpectedKind = routeEvents[0].kind;
  setDriveMode(MODE_ROUTE_REPLAY);
}

void stopAutoDrive() {
  if (driveMode == MODE_ROUTE_LEARN) {
    finishRouteLearning();
  }
  setDriveMode(MODE_REMOTE);
}

void recordRouteSample(RouteSegmentKind observedKind) {
  unsigned long now = millis();
  if (routeLastSampleAt == 0) routeLastSampleAt = now;
  if (now - routeLastSampleAt < ROUTE_RECORD_SAMPLE_MS) return;

  unsigned long ticks = (now - routeLastSampleAt) / ROUTE_RECORD_SAMPLE_MS;
  if (ticks > 8) ticks = 8;
  for (unsigned long i = 0; i < ticks; i++) {
    if (!routeRecordSample(routeEvents, routeEventCount, observedKind)) {
      routeRecordOverflow = true;
      break;
    }
  }
  routeLastSampleAt += ticks * ROUTE_RECORD_SAMPLE_MS;
}

int routeAdjustedSpeed(RouteSegmentKind observedKind) {
  if (driveMode == MODE_ROUTE_LEARN) {
    recordRouteSample(observedKind);
    return min(speed, routeSpeedLimitForMode(RouteRunMode::Learn, observedKind, false));
  }

  if (driveMode == MODE_ROUTE_REPLAY && routeEventCount > 0) {
    unsigned long elapsedMs = millis() - routeReplayStartedAt;
    uint32_t replayTick = elapsedMs / ROUTE_RECORD_SAMPLE_MS;
    int eventIndex = routeEventIndexForTick(routeEvents, routeEventCount, replayTick + ROUTE_REPLAY_LOOKAHEAD_TICKS);
    if (eventIndex >= 0) {
      routeExpectedKind = routeEvents[eventIndex].kind;
    }
    routeObservedKind = observedKind;
    routeReplayMismatch = !routeKindMatches(routeExpectedKind, observedKind);
    int limit = routeSpeedLimitForMode(RouteRunMode::Replay, routeExpectedKind, routeReplayMismatch);
    return min(speed, limit);
  }

  routeReplayMismatch = false;
  routeObservedKind = observedKind;
  return speed;
}

int routeCurrentEventIndex() {
  if (driveMode == MODE_ROUTE_LEARN) {
    return routeEventCount > 0 ? routeEventCount - 1 : -1;
  }

  if (driveMode == MODE_ROUTE_REPLAY && routeEventCount > 0) {
    unsigned long elapsedMs = millis() - routeReplayStartedAt;
    uint32_t replayTick = elapsedMs / ROUTE_RECORD_SAMPLE_MS;
    return routeEventIndexForTick(routeEvents, routeEventCount, replayTick);
  }

  return -1;
}

void appendRouteJson(String &body) {
  body += "[";
  for (int i = 0; i < routeEventCount; i++) {
    if (i > 0) body += ",";
    body += "[\"";
    body += routeKindCode(routeEvents[i].kind);
    body += "\",";
    body += routeEvents[i].durationTicks;
    body += "]";
  }
  body += "]";
}

void handleControlCommand(const String &rxValue) {
  if (rxValue.length() <= 0) return;

  char cmd = rxValue[0];
  applySpeedSuffix(rxValue);
  lastCmdTime = millis();
  ledColorTime = millis();
  currentLedColor = colorForCmd(cmd);
  currentCmd = cmd;

  switch (cmd) {
    case 'F':
    case 'B':
    case 'L':
    case 'R':
    case 'l':
    case 'r':
      runManualCommand(cmd);
      break;
    case 'T':
      setDriveMode(MODE_LINE_TRACK);
      break;
    case 'E':
      startRouteLearning();
      break;
    case 'P':
      startRouteReplay();
      break;
    case 'C':
      clearRouteMemory();
      stopAutoDrive();
      break;
    case 'M':
    case 'S':
      stopAutoDrive();
      break;
    default:
      if (cmd >= '0' && cmd <= '9') {
        speed = cmd == '0' ? 0 : map(cmd - '0', 1, 9, 80, 255);
      }
      break;
  }
}

void handleApiState() {
  int activeSensors = lineTrackingActiveCount(lineSensorMask, IR_SENSOR_COUNT);
  int lineError = lineTrackingPositionError(lineSensorMask, IR_SENSOR_COUNT, lastLineError);
  int stationCount = wifiStationCount();
  String body;
  body.reserve(2200);
  body += "{";
  body += "\"mode\":\"";
  body += driveModeText();
  body += "\",\"status\":\"";
  const char *status = routeStatusText();
  body += status[0] == '\0' ? "IDLE" : status;
  body += "\",\"cmd\":\"";
  body += currentCmd;
  body += "\",\"speed\":";
  body += speed;
  body += ",\"sensorMask\":";
  body += static_cast<int>(lineSensorMask);
  body += ",\"activeSensors\":";
  body += activeSensors;
  body += ",\"lineError\":";
  body += lineError;
  body += ",\"lineQuality\":\"";
  body += dashboardLineQualityCode(dashboardLineQuality(activeSensors, lineError));
  body += "\"";
  body += ",\"lineSearch\":";
  body += lineSearchActive ? "true" : "false";
  body += ",\"searchStep\":";
  body += lineSearchStep;
  body += ",\"searchDirection\":";
  body += lineSearchDirection;
  body += ",\"uptime\":";
  body += millis() / 1000;
  body += ",\"wifiClients\":";
  body += stationCount;
  body += ",\"routeCount\":";
  body += routeEventCount;
  body += ",\"routeTicks\":";
  body += routeTotalTicks(routeEvents, routeEventCount);
  body += ",\"routeIndex\":";
  body += routeCurrentEventIndex();
  body += ",\"mismatch\":";
  body += routeReplayMismatch ? "true" : "false";
  body += ",\"expected\":\"";
  body += routeKindCode(routeExpectedKind);
  body += "\",\"observed\":\"";
  body += routeKindCode(routeObservedKind);
  body += "\",\"ip\":\"";
  body += WiFi.softAPIP().toString();
  body += "\",\"route\":";
  appendRouteJson(body);
  body += "}";

  webServer.send(200, "application/json", body);
}

void handleApiCommand() {
  if (!webServer.hasArg("c") || webServer.arg("c").length() == 0) {
    webServer.send(400, "text/plain", "missing command");
    return;
  }

  handleControlCommand(webServer.arg("c"));
  webServer.send(200, "application/json", "{\"ok\":true}");
}

void handleControlPage() {
  webServer.send_P(200, "text/html; charset=utf-8", WIFI_CONTROL_PAGE_HTML);
}

void setupWifiControl() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_CONTROL_AP_SSID, WIFI_CONTROL_AP_PASSWORD);

  dnsServer.start(WIFI_CONTROL_DNS_PORT, "*", WiFi.softAPIP());

  webServer.on("/", HTTP_GET, handleControlPage);
  webServer.on("/api/state", HTTP_GET, handleApiState);
  webServer.on("/api/cmd", HTTP_GET, handleApiCommand);
  webServer.on("/api/cmd", HTTP_POST, handleApiCommand);
  webServer.on("/generate_204", HTTP_GET, handleControlPage);
  webServer.on("/hotspot-detect.html", HTTP_GET, handleControlPage);
  webServer.on("/connecttest.txt", HTTP_GET, handleControlPage);
  webServer.on("/ncsi.txt", HTTP_GET, handleControlPage);
  webServer.onNotFound(handleControlPage);
  webServer.begin();
}

void drawSmiley() {
  display.clearDisplay();
  display.drawLine(40, 14, 52, 6, SH110X_WHITE);
  display.drawLine(52, 6, 76, 6, SH110X_WHITE);
  display.drawLine(76, 6, 88, 14, SH110X_WHITE);
  display.drawLine(40, 14, 40, 38, SH110X_WHITE);
  display.drawLine(88, 14, 88, 38, SH110X_WHITE);
  display.drawLine(40, 38, 52, 54, SH110X_WHITE);
  display.drawLine(88, 38, 76, 54, SH110X_WHITE);
  display.drawLine(52, 54, 76, 54, SH110X_WHITE);

  display.drawRect(34, 22, 6, 14, SH110X_WHITE);
  display.drawRect(88, 22, 6, 14, SH110X_WHITE);
  display.drawFastHLine(36, 19, 7, SH110X_WHITE);
  display.drawFastHLine(85, 19, 7, SH110X_WHITE);
  display.drawLine(52, 8, 48, 1, SH110X_WHITE);
  display.drawLine(76, 8, 80, 1, SH110X_WHITE);

  display.fillRect(48, 20, 14, 5, SH110X_WHITE);
  display.fillRect(66, 20, 14, 5, SH110X_WHITE);
  display.drawLine(48, 19, 62, 16, SH110X_WHITE);
  display.drawLine(66, 16, 80, 19, SH110X_WHITE);
  display.drawLine(63, 23, 65, 23, SH110X_WHITE);

  display.drawLine(60, 27, 56, 40, SH110X_WHITE);
  display.drawLine(68, 27, 72, 40, SH110X_WHITE);
  display.drawLine(56, 40, 64, 45, SH110X_WHITE);
  display.drawLine(72, 40, 64, 45, SH110X_WHITE);

  display.drawPixel(55, 47, SH110X_WHITE);
  display.drawPixel(56, 48, SH110X_WHITE);
  display.drawFastHLine(57, 49, 14, SH110X_WHITE);
  display.drawPixel(72, 48, SH110X_WHITE);
  display.drawPixel(73, 47, SH110X_WHITE);
  display.drawFastHLine(59, 52, 10, SH110X_WHITE);
  display.display();
}

void drawBootTransformFrame(int frame) {
  display.clearDisplay();
  display.drawRect(4, 4, 120, 56, SH110X_WHITE);
  display.drawFastHLine(8, 14, 112, SH110X_WHITE);
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(12, 7);
  display.print("CAR HUD");
  display.setCursor(86, 7);
  display.print("S3");

  int scanX = 8 + frame * 14;
  if (scanX > 118) scanX = 118;
  display.drawFastVLine(scanX, 17, 36, SH110X_WHITE);
  display.drawFastVLine(scanX + 1, 20, 30, SH110X_WHITE);

  display.setCursor(14, 22);
  if (frame < 2) display.print("BOOT");
  else if (frame < 5) display.print("SENSORS");
  else display.print("READY");

  display.drawRect(14, 40, 100, 6, SH110X_WHITE);
  int progress = (frame + 1) * 96 / BOOT_TRANSFORM_FRAME_COUNT;
  display.fillRect(16, 42, progress, 2, SH110X_WHITE);

  for (int i = 0; i < IR_SENSOR_COUNT; i++) {
    int x = 34 + i * 12;
    display.drawCircle(x, 32, 3, SH110X_WHITE);
    if (frame >= i + 2) {
      display.fillCircle(x, 32, 2, SH110X_WHITE);
    }
  }

  if (frame >= BOOT_TRANSFORM_FRAME_COUNT - 2) {
    display.setCursor(45, 50);
    display.print("ONLINE");
  }
  display.display();
}

void playBootTransformAnimation() {
  for (int frame = 0; frame < BOOT_TRANSFORM_FRAME_COUNT; frame++) {
    drawBootTransformFrame(frame);
    delay(BOOT_TRANSFORM_FRAME_DELAY_MS);
  }
}

void drawArrowUp() {
  display.clearDisplay();
  int cx = 64, cy = 32;
  display.fillRect(cx - 6, cy - 4, 12, 28, SH110X_WHITE);
  display.fillTriangle(cx, cy - 20, cx - 18, cy, cx + 18, cy, SH110X_WHITE);
  display.display();
}

void drawArrowDown() {
  display.clearDisplay();
  int cx = 64, cy = 32;
  display.fillRect(cx - 6, cy - 24, 12, 28, SH110X_WHITE);
  display.fillTriangle(cx, cy + 20, cx - 18, cy, cx + 18, cy, SH110X_WHITE);
  display.display();
}

void drawArrowLeft() {
  display.clearDisplay();
  int cx = 64, cy = 32;
  display.fillRect(cx - 4, cy - 6, 28, 12, SH110X_WHITE);
  display.fillTriangle(cx - 20, cy, cx, cy - 18, cx, cy + 18, SH110X_WHITE);
  display.display();
}

void drawArrowRight() {
  display.clearDisplay();
  int cx = 64, cy = 32;
  display.fillRect(cx - 24, cy - 6, 28, 12, SH110X_WHITE);
  display.fillTriangle(cx + 20, cy, cx, cy - 18, cx, cy + 18, SH110X_WHITE);
  display.display();
}

void drawDashboardArrow(char arrow) {
  int cx = 64;
  int cy = 31;
  switch (arrow) {
    case '^':
      display.fillRect(cx - 4, cy - 1, 8, 18, SH110X_WHITE);
      display.fillTriangle(cx, cy - 16, cx - 14, cy + 1, cx + 14, cy + 1, SH110X_WHITE);
      break;
    case 'v':
      display.fillRect(cx - 4, cy - 17, 8, 18, SH110X_WHITE);
      display.fillTriangle(cx, cy + 16, cx - 14, cy - 1, cx + 14, cy - 1, SH110X_WHITE);
      break;
    case '<':
      display.fillRect(cx - 1, cy - 4, 22, 8, SH110X_WHITE);
      display.fillTriangle(cx - 16, cy, cx + 1, cy - 14, cx + 1, cy + 14, SH110X_WHITE);
      break;
    case '>':
      display.fillRect(cx - 21, cy - 4, 22, 8, SH110X_WHITE);
      display.fillTriangle(cx + 16, cy, cx - 1, cy - 14, cx - 1, cy + 14, SH110X_WHITE);
      break;
    default:
      {
        DashboardIdleFace face = dashboardIdleFace();
        display.drawCircle(face.centerX, face.centerY, face.radius, SH110X_WHITE);
        display.drawCircle(face.centerX, face.centerY, face.radius - 1, SH110X_WHITE);
        display.fillRect(face.centerX - 9, face.centerY - 5, 6, 3, SH110X_WHITE);
        display.fillRect(face.centerX + 3, face.centerY - 5, 6, 3, SH110X_WHITE);
        DashboardSmileMouth smile = dashboardIdleSmileMouth();
        display.drawPixel(smile.leftX, smile.cornerY, SH110X_WHITE);
        display.drawPixel(smile.leftX + 1, smile.cornerY + 1, SH110X_WHITE);
        display.drawPixel(smile.leftX + 2, smile.cornerY + 2, SH110X_WHITE);
        display.drawFastHLine(58, smile.middleY, smile.middleWidth, SH110X_WHITE);
        display.drawPixel(smile.rightX - 2, smile.cornerY + 2, SH110X_WHITE);
        display.drawPixel(smile.rightX - 1, smile.cornerY + 1, SH110X_WHITE);
        display.drawPixel(smile.rightX, smile.cornerY, SH110X_WHITE);
      }
      break;
  }
}

void drawLineSensorMiniBar(int x, int y) {
  for (int i = 0; i < IR_SENSOR_COUNT; i++) {
    int px = x + i * 9;
    display.drawRect(px, y, 6, 6, SH110X_WHITE);
    if (dashboardLineSensorSlotActive(lineSensorMask, i, IR_SENSOR_COUNT)) {
      display.fillRect(px + 1, y + 1, 4, 4, SH110X_WHITE);
    }
  }
}

void printTwoDigits(unsigned long value) {
  if (value < 10) display.print('0');
  display.print(value);
}

void drawDashboard(char displayCmd) {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.print(deviceConnected ? "BLE ON" : "BLE OFF");
  display.setCursor(74, 0);
  display.print("SPD ");
  display.print(speed);
  display.drawFastHLine(0, 10, 128, SH110X_WHITE);

  display.setCursor(0, 14);
  display.print(driveModeText());
  display.setCursor(0, 27);
  display.print("CMD ");
  display.print(displayCmd);

  if (isAutoDriveMode()) {
    display.setCursor(88, 14);
    display.print(routeStatusText());
  }

  drawDashboardArrow(dashboardArrowForCommand(displayCmd));

  display.drawFastHLine(0, 47, 128, SH110X_WHITE);
  display.setCursor(0, 54);
  display.print("LINE");
  drawLineSensorMiniBar(29, 53);

  unsigned long totalSeconds = millis() / 1000;
  unsigned long minutes = (totalSeconds / 60) % 100;
  unsigned long seconds = totalSeconds % 60;
  display.setCursor(80, 54);
  if (driveMode == MODE_ROUTE_LEARN || driveMode == MODE_ROUTE_REPLAY) {
    display.print("RT ");
    if (routeEventCount < 10) display.print('0');
    display.print(routeEventCount);
  } else {
    display.print("UP ");
    printTwoDigits(minutes);
    display.print(':');
    printTwoDigits(seconds);
  }

  display.display();
}

void drawLabel(const char *label, const char *subLabel) {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(2);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(label, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((128 - w) / 2, 17);
  display.print(label);
  display.setTextSize(1);
  display.getTextBounds(subLabel, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((128 - w) / 2, 43);
  display.print(subLabel);
  display.display();
}

void updateDisplay() {
  unsigned long now = millis();
  char displayCmd = currentCmd;
  if (isAutoDriveMode()) displayCmd = currentCmd;
  else if (millis() - lastCmdTime > 500) displayCmd = 'S';
  if (displayCmd == prevDisplayCmd && !dashboardShouldRefresh(now, lastDashboardRefresh)) return;
  prevDisplayCmd = displayCmd;
  lastDashboardRefresh = now;
  drawDashboard(displayCmd);
}

void runLineTracking() {
  unsigned long now = millis();
  int activeCount = lineTrackingActiveCount(lineSensorMask, IR_SENSOR_COUNT);
  int error = lineTrackingPositionError(lineSensorMask, IR_SENSOR_COUNT, lastLineError);
  RouteSegmentKind observedKind = routeKindForLine(activeCount, error);
  int targetSpeed = routeAdjustedSpeed(observedKind);

  if (activeCount == 0) {
    runLineSearch();
    return;
  }

  if (lineTrackingShouldSearchWide(lineSensorMask, IR_SENSOR_COUNT)) {
    lastLineSeenAt = now;
    lastSeenLineActiveCount = activeCount;
    runWideLineSearch();
    return;
  }

  if (activeCount < 4) {
    lastSeenLineError = error;
    if (error != 0) {
      lastDirectionalLineError = error;
      lastDirectionalLineSeenAt = now;
    }
  }
  lastSeenLineActiveCount = activeCount;
  lastLineSeenAt = now;
  resetLineSearch();

  int derivative = error - lastLineError;
  if (activeCount < 4) lastLineError = error;

  int base = lineTrackingBaseSpeed(targetSpeed, activeCount, error);
  SearchMotorCommand command = lineTrackingMotorCommand(base, error, derivative, activeCount);
  motor.set(command.left, command.right);
}

void runManualCommand(char cmd) {
  setDriveMode(MODE_REMOTE);
  currentCmd = cmd;

  DriveMotorCommand command = manualDriveMotorCommand(cmd, speed);
  motor.set(command.left, command.right);
}

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
  }
  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    stopAutoDrive();
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String rxValue = pCharacteristic->getValue();
    handleControlCommand(rxValue);
  }
};

void setup() {
  rgb.begin();
  rgb.clear();
  rgb.show();

  motor.init();
  initIrSensors();
  loadRouteMemory();

  Wire.begin(11, 12);
  display.begin(0x3C, true);
  // If screen is blank, try 0x3D instead:
  // display.begin(0x3D, true);
  playBootTransformAnimation();
  setupWifiControl();

  BLEDevice::init("ESP32-S3-Car");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pTxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();
  pServer->getAdvertising()->start();
}

void loop() {
  updateLineSensorMask();
  dnsServer.processNextRequest();
  webServer.handleClient();

  if (isAutoDriveMode()) {
    runLineTracking();
  } else if (millis() - lastCmdTime > 500) {
    motor.stop();
    currentCmd = 'S';
  }

  updateDisplay();

  // RGB LED feedback
  if (dashboardStatusLedShouldBlink(deviceConnected, wifiStationCount())) {
    rgb.setPixelColor(0, (millis() / 500) % 2
      ? Adafruit_NeoPixel::Color(0, 0, 0)
      : Adafruit_NeoPixel::Color(10, 10, 10));
    rgb.show();
  } else if (millis() - ledColorTime < LED_DURATION) {
    rgb.setPixelColor(0, currentLedColor);
    rgb.show();
  } else {
    rgb.setPixelColor(0, Adafruit_NeoPixel::Color(0, 0, 0));
    rgb.show();
  }

  // Disconnect handling
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    oldDeviceConnected = false;
  }
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = true;
  }

  delay(20);
}
