#include "Co2Sensor.hpp"

Co2Sensor::Co2Sensor()
    : _co2(400), _temperature(20.0), _humidity(50.0), _dataValid(false),
      _lastMeasurementTime(0), _measurementStarted(false) {}

bool Co2Sensor::begin() {
  Wire.begin(); // Initialize I2C on Port A
  _scd40.begin(Wire);

  // Stop potentially previously started measurement
  uint16_t error = _scd40.stopPeriodicMeasurement();
  if (error) {
    Serial.print("Error stopping SCD40 measurement: ");
    Serial.println(error);
  }

  delay(100);

  // Start periodic measurement
  error = _scd40.startPeriodicMeasurement();
  if (error) {
    Serial.print("Error starting SCD40 measurement: ");
    Serial.println(error);
    return false;
  }

  Serial.println("SCD40 initialized successfully");
  _measurementStarted = true;
  _lastMeasurementTime = millis();
  return true;
}

void Co2Sensor::update() {
  if (!_measurementStarted)
    return;

  unsigned long now = millis();

  // SCD40 provides new data every ~5 seconds
  if (now - _lastMeasurementTime >= MEASUREMENT_INTERVAL) {
    uint16_t co2;
    float temperature;
    float humidity;
    bool isDataReady = false;

    uint16_t error = _scd40.getDataReadyFlag(isDataReady);
    if (error) {
      Serial.print("Error checking data ready: ");
      Serial.println(error);
      return;
    }

    if (isDataReady) {
      error = _scd40.readMeasurement(co2, temperature, humidity);
      if (error) {
        Serial.print("Error reading measurement: ");
        Serial.println(error);
      } else if (co2 != 0) {
        _co2 = co2;
        _temperature = temperature;
        _humidity = humidity;
        _dataValid = true;
        _lastMeasurementTime = now;

        static uint16_t logCounter = 0;
        if (++logCounter >= 10000)
          logCounter = 0;

        Serial.printf("CO2: %d ppm, Temp: %.1f C, Humidity: %.1f %% #%d\n",
                      _co2, _temperature, _humidity, logCounter);
      }
    }
  }
}

uint16_t Co2Sensor::getCo2() { return _co2; }

float Co2Sensor::getTemperature() { return _temperature; }

float Co2Sensor::getHumidity() { return _humidity; }

bool Co2Sensor::isDataValid() { return _dataValid; }

void Co2Sensor::setTemperatureOffset(float offsetC) {
  uint16_t offsetTicks = (uint16_t)(offsetC * 374.5); // Convert to sensor ticks
  uint16_t error = _scd40.stopPeriodicMeasurement();
  delay(500);

  error = _scd40.setTemperatureOffset(offsetTicks);
  if (error) {
    Serial.print("Error setting temperature offset: ");
    Serial.println(error);
  }

  delay(20);
  error = _scd40.startPeriodicMeasurement();
  if (error) {
    Serial.print("Error restarting measurement: ");
    Serial.println(error);
  }
}
