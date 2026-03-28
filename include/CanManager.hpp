#ifndef CAN_MANAGER_HPP
#define CAN_MANAGER_HPP

#include <Arduino.h>
#include <ESP32-TWAI-CAN.hpp>

class CanManager {
public:
  CanManager();
  bool begin();

  // Non-blocking update
  void update();

  // Data getters
  int16_t getCoolantTemp();  // Celsius, -40 to 215
  uint8_t getThrottlePos();  // Percentage, 0-100
  float getBatteryVoltage(); // Volts
  uint16_t getRpm();         // Engine RPM, 0-16383

  bool isDataValid();

  // Debug control
  void setDebugEnabled(bool enabled);
  bool isDebugEnabled() const;

private:
  // OBD2 PIDs
  static constexpr uint8_t PID_COOLANT_TEMP = 0x05;
  static constexpr uint8_t PID_ENGINE_RPM = 0x0C;
  static constexpr uint8_t PID_THROTTLE_POS = 0x49;
  static constexpr uint8_t PID_CONTROL_MODULE_VOLTAGE = 0x42;

  // CAN identifiers
  static constexpr uint32_t OBD2_REQUEST_ID = 0x7DF;
  static constexpr uint32_t OBD2_RESPONSE_ID = 0x7E8;

  // Data cache
  int16_t _coolantTemp;
  uint8_t _throttlePos;
  float _batteryVoltage;
  uint16_t _engineRpm;

  // State management
  unsigned long _lastResponseTime; // Last response from any PID
  bool _dataValid;

  // Per-PID last update tracking for hybrid passive/active mode
  unsigned long _lastCoolantTempUpdate;
  unsigned long _lastThrottlePosUpdate;
  unsigned long _lastBatteryVoltageUpdate;
  unsigned long _lastEngineRpmUpdate;

  // If a PID hasn't been updated for this duration, actively request it
  static constexpr unsigned long PID_UPDATE_TIMEOUT = 1000; // ms
  static constexpr unsigned long RESPONSE_TIMEOUT = 500;    // ms
  static constexpr unsigned long DEBUG_INTERVAL = 2000;     // ms
  unsigned long _lastDebugTime = 0;

  bool _debugEnabled = false; // Debug flag

  // Helper methods
  void checkAndRequestPid(uint8_t pid, unsigned long &lastUpdate,
                          const char *name);
  void requestPid(uint8_t pid);
  void processResponse();
  void parseResponse(const CanFrame &frame);
};

#endif // CAN_MANAGER_HPP
