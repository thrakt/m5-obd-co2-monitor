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

  // Timing
  static constexpr unsigned long RESPONSE_TIMEOUT = 200; // ms per request
  static constexpr unsigned long DATA_TIMEOUT = 2000;    // ms until data invalid
  static constexpr unsigned long DEBUG_INTERVAL = 2000;  // ms

  // PID request state machine
  enum class RequestState { IDLE, WAITING_RESPONSE };

  // Weighted PID sequence:
  // RPM x3, THROTTLE x3, COOLANT x1, VOLTAGE x1 per cycle
  static constexpr uint8_t PID_SEQUENCE[] = {
      PID_ENGINE_RPM,           // high priority
      PID_THROTTLE_POS,         // high priority
      PID_ENGINE_RPM,           // high priority
      PID_THROTTLE_POS,         // high priority
      PID_ENGINE_RPM,           // high priority
      PID_THROTTLE_POS,         // high priority
      PID_COOLANT_TEMP,         // low priority
      PID_CONTROL_MODULE_VOLTAGE // low priority
  };
  static constexpr uint8_t PID_SEQUENCE_LEN =
      sizeof(PID_SEQUENCE) / sizeof(PID_SEQUENCE[0]);

  // State
  RequestState _requestState;
  uint8_t _pidIndex;           // Current index in PID_SEQUENCE
  unsigned long _requestTime;  // When the current request was sent

  // Data cache
  int16_t _coolantTemp;
  uint8_t _throttlePos;
  float _batteryVoltage;
  uint16_t _engineRpm;

  // Validity tracking
  unsigned long _lastResponseTime;
  bool _dataValid;

  unsigned long _lastDebugTime = 0;
  bool _debugEnabled = false;

  // Helper methods
  void sendRequest(uint8_t pid);
  bool processResponse();       // Returns true if a matching response was received
  void parseResponse(const CanFrame &frame);
  uint8_t currentPid() const { return PID_SEQUENCE[_pidIndex]; }
  void advancePid() { _pidIndex = (_pidIndex + 1) % PID_SEQUENCE_LEN; }
};

#endif // CAN_MANAGER_HPP
