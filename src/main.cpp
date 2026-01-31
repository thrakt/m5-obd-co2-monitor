#include "CanManager.hpp"
#include "Co2Sensor.hpp"
#include "Display.hpp"
#include "LedManager.hpp"
#include "SoundManager.hpp"
#include "ThrottleGraph.hpp"
#include <M5Unified.h>

// Component instances
Display display;
CanManager canManager;
Co2Sensor co2Sensor;
LedManager ledManager;
SoundManager soundManager;
ThrottleGraph throttleGraph;

// State management
bool isHeaterReady = false;
bool isCo2WarningActive = false;
unsigned long startupTime = 0;

// Constants
const unsigned long STARTUP_GRACE_PERIOD = 10000; // 10 seconds

// Heater thresholds
const int16_t HEATER_READY_TEMP = 75;
const int16_t HEATER_COLD_TEMP = 60;

// CO2 thresholds
const uint16_t CO2_WARNING_THRESHOLD = 1500;

bool isStartupGracePeriod() {
  return (millis() - startupTime) < STARTUP_GRACE_PERIOD;
}

void setup() {
  // Initialize M5Stack
  auto cfg = M5.config();
  M5.begin(cfg);

  Serial.begin(115200);
  delay(500);

  Serial.println("\n\n=== M5Stack CoreS3 OBD-CO2 Monitor ===");
  Serial.println("Initializing components...");

  startupTime = millis();

  // Initialize display
  display.begin();
  display.clear();
  Serial.println("Display initialized");

  // Initialize CAN Manager
  if (!canManager.begin()) {
    Serial.println("WARNING: CAN initialization failed!");
  }

  // Initialize CO2 Sensor
  if (!co2Sensor.begin()) {
    Serial.println("WARNING: CO2 sensor initialization failed!");
  }

  // Initialize LED Manager
  ledManager.begin();

  // Initialize Sound Manager
  soundManager.begin();

  // Play a test sound on startup before initializing the visualizer
  Serial.println("Playing startup sound...");
  soundManager.playPon();

  // Wait for the asynchronous sound to finish
  unsigned long soundStartTime = millis();
  while (soundManager.isPlaying() &&
         millis() - soundStartTime < 1000) { // Timeout after 1 sec
    soundManager.update();
    delay(10);
  }
  Serial.println("Startup sound finished.");

  // Initialize Throttle Graph
  const uint16_t GRAPH_HISTORY_SIZE = 320;
  const uint16_t GRAPH_UPDATE_INTERVAL_MS = 500;
  throttleGraph.begin(&M5.Display, 0, 190, 320, 50, GRAPH_HISTORY_SIZE,
                      GRAPH_UPDATE_INTERVAL_MS);
  Serial.printf("Throttle Graph: %d data points, %.1f sec history\n",
                GRAPH_HISTORY_SIZE, throttleGraph.getHistoryDurationSeconds());

  Serial.println("All components initialized");
  Serial.println("===================================\n");
}

void loop() {
  M5.update();

  // Update all components (non-blocking)
  canManager.update();
  co2Sensor.update();
  soundManager.update();

  // Get current data
  float batteryVoltage = canManager.getBatteryVoltage();
  uint16_t co2 = co2Sensor.getCo2();
  int16_t coolantTemp = canManager.getCoolantTemp();
  uint8_t throttlePos = canManager.getThrottlePos();
  float cabinTemp = co2Sensor.getTemperature();
  float cabinHumidity = co2Sensor.getHumidity();

  // Update display zones
  display.updateHeader(batteryVoltage, co2);
  display.updateThrottleGauge(throttlePos);
  display.updateCoolantTemp(coolantTemp);
  display.updateCabinEnv(cabinTemp, cabinHumidity);
  // Add current throttle data to graph and update visualizer
  static unsigned long lastGraphUpdate = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastGraphUpdate >= 500) { // Update every 500ms
    throttleGraph.addData(throttlePos);
    throttleGraph.draw();
    display.updateThrottleGraph();
    lastGraphUpdate = currentTime;
  }

  // Push all updates to the screen at once
  static unsigned long lastDisplayUpdate = 0;
  if (currentTime - lastDisplayUpdate >= 33) {
    display.updateDisplay();
    lastDisplayUpdate = currentTime;
  }

  // Update LED (non-blocking breathing effect)
  if (isStartupGracePeriod()) {
    ledManager.clear();
  } else {
    ledManager.update(co2);
  }

  // === HEATER READY STATE MACHINE ===
  if (!isStartupGracePeriod()) {
    // Rule A: Enter heater ready state
    if (!isHeaterReady && coolantTemp >= HEATER_READY_TEMP) {
      isHeaterReady = true;
      Serial.println(">> Heater ready! Temperature reached 75°C");
      soundManager.playPon();
    }

    // Rule B: Exit heater ready state
    if (isHeaterReady && coolantTemp <= HEATER_COLD_TEMP) {
      isHeaterReady = false;
      Serial.println(">> Cooling detected! Temperature dropped below 60°C");
      soundManager.playPonPon();
    }
  }

  // === CO2 WARNING SYSTEM ===
  // Rule A: Enter CO2 warning state
  if (!isStartupGracePeriod() && !isCo2WarningActive &&
      co2 >= CO2_WARNING_THRESHOLD) {
    isCo2WarningActive = true;
    Serial.printf(">> CO2 WARNING! Level: %d ppm\n", co2);
    soundManager.playBeee();
  }

  // Rule B: Exit CO2 warning state
  if (isCo2WarningActive && co2 < CO2_WARNING_THRESHOLD) {
    isCo2WarningActive = false;
    Serial.println(">> CO2 level is back to normal.");
  }

  // Small delay to avoid overwhelming the system
  delay(10);
}
