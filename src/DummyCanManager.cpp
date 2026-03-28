#include "DummyCanManager.hpp"

DummyCanManager::DummyCanManager()
    : _coolantTemp(-40), _throttlePos(0), _batteryVoltage(0.0), _engineRpm(0),
      _lastResponseTime(0), _dataValid(false) {}

bool DummyCanManager::begin() {
  Serial.println("DummyCanManager initialized (SIMULATION)");
  return true;
}

void DummyCanManager::update() {
  unsigned long now = millis();

  // SIMULATION MODE
  _dataValid = true;
  _lastResponseTime = now;

  // Simulate Throttle: 0 to 100% cycle
  _throttlePos = (now / 100) % 101;

  // Simulate Engine RPM: 700 to 7000 rpm cycle
  _engineRpm = 700 + (uint16_t)((now / 50) % 6301);

  // Simulate Coolant Temp: 55 to 105 C cycle
  _coolantTemp = 55 + ((now / 1000) % 51);

  // Simulate Voltage: 13.0 to 14.5 V cycle
  _batteryVoltage = 13.0 + ((now % 5000) / 5000.0) * 1.5;

  // Print simulation status occasionally (only if debug is enabled)
  if (_debugEnabled && (now - _lastDebugTime >= DEBUG_INTERVAL)) {
    Serial.printf("[SIM] RPM: %d, Throttle: %d%%, Temp: %d C, Volt: %.2f V\n",
                  _engineRpm, _throttlePos, _coolantTemp, _batteryVoltage);
    _lastDebugTime = now;
  }
}

int16_t DummyCanManager::getCoolantTemp() { return _coolantTemp; }

uint8_t DummyCanManager::getThrottlePos() { return _throttlePos; }

float DummyCanManager::getBatteryVoltage() { return _batteryVoltage; }

uint16_t DummyCanManager::getRpm() { return _engineRpm; }

bool DummyCanManager::isDataValid() { return _dataValid; }

void DummyCanManager::setDebugEnabled(bool enabled) { _debugEnabled = enabled; }

bool DummyCanManager::isDebugEnabled() const { return _debugEnabled; }
