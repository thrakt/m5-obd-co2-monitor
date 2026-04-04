#include "CanManager.hpp"
#include "driver/twai.h"

namespace {
const int CAN_TX_PIN = 17;
const int CAN_RX_PIN = 18;
} // namespace

// Static member definition
constexpr uint8_t CanManager::PID_SEQUENCE[];

CanManager::CanManager()
    : _requestState(RequestState::IDLE), _pidIndex(0), _requestTime(0),
      _coolantTemp(-40), _throttlePos(0), _batteryVoltage(0.0), _engineRpm(0),
      _lastResponseTime(0), _dataValid(false) {}

bool CanManager::begin() {
  if (!ESP32Can.begin(ESP32Can.convertSpeed(500), CAN_TX_PIN, CAN_RX_PIN, 10,
                      10)) {
    Serial.println("CAN initialization failed!");
    return false;
  }
  Serial.println("CAN initialized successfully");
  return true;
}

void CanManager::update() {
  unsigned long now = millis();

  // --- Data validity timeout ---
  if (_dataValid && (now - _lastResponseTime > DATA_TIMEOUT)) {
    _dataValid = false;
    Serial.println("[CAN] Data timeout - resetting values");
    _coolantTemp = -40;
    _throttlePos = 0;
    _batteryVoltage = 0.0;
    _engineRpm = 0;
  }

  // --- Debug status logging ---
  if (_debugEnabled && now - _lastDebugTime >= DEBUG_INTERVAL) {
    twai_status_info_t status;
    if (twai_get_status_info(&status) == ESP_OK) {
      const char *stateStr = "UNKNOWN";
      switch (status.state) {
      case TWAI_STATE_STOPPED:   stateStr = "STOPPED";   break;
      case TWAI_STATE_RUNNING:   stateStr = "RUNNING";   break;
      case TWAI_STATE_BUS_OFF:   stateStr = "BUS_OFF";   break;
      case TWAI_STATE_RECOVERING:stateStr = "RECOVERING";break;
      }
      Serial.printf("[CAN STATUS] State: %s, TX Err: %d, RX Err: %d, "
                    "TX Que: %d, RX Que: %d, Missed: %d\n",
                    stateStr, status.tx_error_counter, status.rx_error_counter,
                    status.msgs_to_tx, status.msgs_to_rx,
                    status.rx_missed_count);
    } else {
      Serial.println("[CAN STATUS] Failed to get status");
    }
    _lastDebugTime = now;
  }

  // --- State machine ---
  switch (_requestState) {
  case RequestState::IDLE:
    // Send request for the current PID in the weighted sequence
    sendRequest(currentPid());
    _requestTime = now;
    _requestState = RequestState::WAITING_RESPONSE;
    break;

  case RequestState::WAITING_RESPONSE:
    if (processResponse()) {
      // Got a matching response; advance to next PID
      advancePid();
      _requestState = RequestState::IDLE;
    } else if (now - _requestTime > RESPONSE_TIMEOUT) {
      // Timeout: skip this PID and move on
      if (_debugEnabled) {
        Serial.printf("[CAN] Timeout waiting for PID 0x%02X - skipping\n",
                      currentPid());
      }
      advancePid();
      _requestState = RequestState::IDLE;
    }
    break;
  }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void CanManager::sendRequest(uint8_t pid) {
  CanFrame req = {};
  req.identifier = OBD2_REQUEST_ID;
  req.extd = 0;
  req.data_length_code = 8;
  req.data[0] = 0x02; // Number of additional bytes
  req.data[1] = 0x01; // Mode 01 (show current data)
  req.data[2] = pid;
  // Padding bytes
  req.data[3] = 0x00;
  req.data[4] = 0x00;
  req.data[5] = 0x00;
  req.data[6] = 0x00;
  req.data[7] = 0x00;

  if (_debugEnabled) {
    Serial.printf("[CAN TX] Requesting PID 0x%02X\n", pid);
  }

  ESP32Can.writeFrame(req);
}

bool CanManager::processResponse() {
  CanFrame frame;
  while (ESP32Can.readFrame(frame, 0)) {
    // Log all received frames in debug mode
    if (_debugEnabled) {
      Serial.printf("[CAN RX] ID:0x%08X %s DLC:%d Data:",
                    frame.identifier, frame.extd ? "EXT" : "STD",
                    frame.data_length_code);
      for (int i = 0; i < frame.data_length_code; i++) {
        Serial.printf(" %02X", frame.data[i]);
      }
      Serial.println();
    }

    // Accept standard OBD2 response IDs (0x7E8-0x7EF)
    if (frame.identifier == OBD2_RESPONSE_ID ||
        (frame.identifier >= 0x7E8 && frame.identifier <= 0x7EF)) {

      // Only handle positive responses (Mode 01 response = 0x41)
      if (frame.data_length_code >= 3 && frame.data[1] == 0x41 &&
          frame.data[2] == currentPid()) {
        parseResponse(frame);
        _lastResponseTime = millis();
        _dataValid = true;
        return true; // Matched the PID we requested
      }
    }
  }
  return false;
}

void CanManager::parseResponse(const CanFrame &frame) {
  uint8_t pid = frame.data[2];

  switch (pid) {
  case PID_COOLANT_TEMP:
    if (frame.data_length_code >= 4) {
      _coolantTemp = (int16_t)frame.data[3] - 40; // Formula: A - 40
      if (_debugEnabled) {
        Serial.printf("[CAN] Coolant Temp: %d C\n", _coolantTemp);
      }
    }
    break;

  case PID_ENGINE_RPM:
    if (frame.data_length_code >= 5) {
      // Formula: ((A * 256) + B) / 4
      _engineRpm = ((uint16_t)frame.data[3] * 256 + frame.data[4]) / 4;
      if (_debugEnabled) {
        Serial.printf("[CAN] Engine RPM: %d\n", _engineRpm);
      }
    }
    break;

  case PID_THROTTLE_POS:
    if (frame.data_length_code >= 4) {
      // Convert to 0-100% using standard OBD-II formula
      uint8_t rawThrottle = (frame.data[3] * 100) / 255;

      // Remap 15-75 range to 0-100%
      if (rawThrottle <= 15) {
        _throttlePos = 0;
      } else if (rawThrottle >= 75) {
        _throttlePos = 100;
      } else {
        _throttlePos = ((rawThrottle - 15) * 100) / 60;
      }

      if (_debugEnabled) {
        Serial.printf("[CAN] Throttle: raw=%d%%, mapped=%d%%\n",
                      rawThrottle, _throttlePos);
      }
    }
    break;

  case PID_CONTROL_MODULE_VOLTAGE:
    if (frame.data_length_code >= 5) {
      uint16_t raw = (frame.data[3] << 8) | frame.data[4];
      _batteryVoltage = raw / 1000.0; // Formula: ((A * 256) + B) / 1000
      if (_debugEnabled) {
        Serial.printf("[CAN] Voltage: %.2f V\n", _batteryVoltage);
      }
    }
    break;

  default:
    if (_debugEnabled) {
      Serial.printf("[CAN] Unknown PID: 0x%02X\n", pid);
    }
    break;
  }
}

// ---------------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------------

int16_t CanManager::getCoolantTemp()  { return _coolantTemp; }
uint8_t CanManager::getThrottlePos()  { return _throttlePos; }
float   CanManager::getBatteryVoltage() { return _batteryVoltage; }
uint16_t CanManager::getRpm()         { return _engineRpm; }
bool    CanManager::isDataValid()     { return _dataValid; }
void    CanManager::setDebugEnabled(bool enabled) { _debugEnabled = enabled; }
bool    CanManager::isDebugEnabled() const { return _debugEnabled; }
