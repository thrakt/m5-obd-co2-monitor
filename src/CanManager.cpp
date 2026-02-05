#include "CanManager.hpp"
#include "driver/twai.h"

namespace {
const int CAN_TX_PIN = 17;
const int CAN_RX_PIN = 18;
} // namespace

CanManager::CanManager()
    : _coolantTemp(-40), _throttlePos(0), _batteryVoltage(0.0),
      _lastRequestTime(0), _lastResponseTime(0), _currentPidIndex(0),
      _dataValid(false) {}

bool CanManager::begin() {
  // Initialize CAN on UART Port C (TX=18, RX=17)
  // Using ESP32-TWAI-CAN library
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

  // Check for timeout
  if (_dataValid && (now - _lastResponseTime > RESPONSE_TIMEOUT * 3)) {
    _dataValid = false;
    Serial.println("CAN data timeout");
    _coolantTemp = -40; // Reset values
    _throttlePos = 0;
    _batteryVoltage = 0.0;
  }

  // Debug Status Logging
  if (_debugEnabled && now - _lastDebugTime >= DEBUG_INTERVAL) {
    twai_status_info_t status;
    if (twai_get_status_info(&status) == ESP_OK) {
      const char *stateStr = "UNKNOWN";
      switch (status.state) {
      case TWAI_STATE_STOPPED:
        stateStr = "STOPPED";
        break;
      case TWAI_STATE_RUNNING:
        stateStr = "RUNNING";
        break;
      case TWAI_STATE_BUS_OFF:
        stateStr = "BUS_OFF";
        break;
      case TWAI_STATE_RECOVERING:
        stateStr = "RECOVERING";
        break;
      }
      Serial.printf("[CAN STATUS] State: %s, TX Err: %d, RX Err: %d, TX Que: "
                    "%d, RX Que: %d, Missed: %d\n",
                    stateStr, status.tx_error_counter, status.rx_error_counter,
                    status.msgs_to_tx, status.msgs_to_rx,
                    status.rx_missed_count);
    } else {
      Serial.println("[CAN STATUS] Failed to get status");
    }
    _lastDebugTime = now;
  }

  // Send periodic requests (Only if RUNNING to avoid filling queue when BusOff)
  // For debugging "flow", we continue to request, but check status first
  // optionally.
  if (now - _lastRequestTime >= REQUEST_INTERVAL) {
    uint8_t pids[] = {PID_COOLANT_TEMP, PID_THROTTLE_POS,
                      PID_CONTROL_MODULE_VOLTAGE};
    requestPid(pids[_currentPidIndex]);
    _currentPidIndex = (_currentPidIndex + 1) % 3;
    _lastRequestTime = now;
  }

  // Process responses
  processResponse();
}

int16_t CanManager::getCoolantTemp() { return _coolantTemp; }

uint8_t CanManager::getThrottlePos() { return _throttlePos; }

float CanManager::getBatteryVoltage() { return _batteryVoltage; }

bool CanManager::isDataValid() { return _dataValid; }

void CanManager::setDebugEnabled(bool enabled) { _debugEnabled = enabled; }

bool CanManager::isDebugEnabled() const { return _debugEnabled; }

// ... (previous content)

void CanManager::requestPid(uint8_t pid) {
  CanFrame frame;
  frame.identifier = OBD2_REQUEST_ID;
  frame.extd = 0;
  frame.data_length_code = 8;
  frame.data[0] = 0x02; // Number of additional bytes
  frame.data[1] = 0x01; // Mode 01 (Show current data)
  frame.data[2] = pid;  // PID
  frame.data[3] = 0x00;
  frame.data[4] = 0x00;
  frame.data[5] = 0x00;
  frame.data[6] = 0x00;
  frame.data[7] = 0x00;

  if (ESP32Can.writeFrame(frame)) {
    if (_debugEnabled) {
      Serial.printf("[CAN] Request sent: PID=0x%02X\n", pid);
    }
  } else {
    if (_debugEnabled) {
      Serial.printf("[CAN] Failed to send request: PID=0x%02X (Queue Full or "
                    "Bus Error)\n",
                    pid);
    }
  }
}

void CanManager::processResponse() {
  CanFrame frame;
  while (ESP32Can.readFrame(frame, 0)) {
    // Verbose logging for debugging "flow"
    if (_debugEnabled) {
      Serial.printf("[CAN RX] ID:0x%08X %s DLC:%d Data:", frame.identifier,
                    frame.extd ? "EXT" : "STD", frame.data_length_code);
      for (int i = 0; i < frame.data_length_code; i++) {
        Serial.printf(" %02X", frame.data[i]);
      }
      Serial.println();
    }

    if (frame.identifier == OBD2_RESPONSE_ID ||
        (frame.identifier >= 0x7E8 && frame.identifier <= 0x7EF)) {
      parseResponse(frame);
      _lastResponseTime = millis();
      _dataValid = true;
    }
  }
}

void CanManager::parseResponse(const CanFrame &frame) {
  if (frame.data_length_code < 3)
    return;

  // Check for positive response (Mode + 0x40)
  if (frame.data[1] != 0x41) {
    if (_debugEnabled) {
      Serial.printf("[CAN] Ignored response: Mode=0x%02X (Expected 0x41)\n",
                    frame.data[1]);
    }
    return;
  }

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

  case PID_THROTTLE_POS:
    if (frame.data_length_code >= 4) {
      // まず標準的なOBD-IIの計算式で0-100の範囲に変換
      uint8_t rawThrottle = (frame.data[3] * 100) / 255;

      // 15-75の範囲を0-100%に再マッピング
      // 15以下は0%、75以上は100%にクランプ
      if (rawThrottle <= 15) {
        _throttlePos = 0;
      } else if (rawThrottle >= 75) {
        _throttlePos = 100;
      } else {
        // 線形変換: (rawThrottle - 15) / (75 - 15) * 100
        _throttlePos = ((rawThrottle - 15) * 100) / 60;
      }

      if (_debugEnabled) {
        Serial.printf("[CAN] Throttle: raw=%d%%, mapped=%d%%\n", rawThrottle,
                      _throttlePos);
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
      Serial.printf("[CAN] Parsed unknown PID: 0x%02X\n", pid);
    }
    break;
  }
}
