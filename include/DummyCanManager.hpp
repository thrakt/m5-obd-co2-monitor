#ifndef DUMMY_CAN_MANAGER_HPP
#define DUMMY_CAN_MANAGER_HPP

#include <Arduino.h>

class DummyCanManager {
public:
  DummyCanManager();
  bool begin();

  // Non-blocking update (Simulates data changes)
  void update();

  // Data getters
  int16_t getCoolantTemp();  // Celsius
  uint8_t getThrottlePos();  // Percentage
  float getBatteryVoltage(); // Volts

  bool isDataValid();

private:
  // Data cache
  int16_t _coolantTemp;
  uint8_t _throttlePos;
  float _batteryVoltage;

  // State management
  unsigned long _lastResponseTime;
  bool _dataValid;

  static constexpr unsigned long DEBUG_INTERVAL = 2000; // ms
  unsigned long _lastDebugTime = 0;
};

#endif // DUMMY_CAN_MANAGER_HPP
