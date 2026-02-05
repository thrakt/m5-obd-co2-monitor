#ifndef CO2_SENSOR_HPP
#define CO2_SENSOR_HPP

#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2CScd4x.h>

class Co2Sensor {
public:
    Co2Sensor();
    bool begin();
    
    // Non-blocking update
    void update();
    
    // Data getters
    uint16_t getCo2();        // ppm
    float getTemperature();   // Celsius
    float getHumidity();      // %RH
    
    bool isDataValid();
    
    // Configuration
    void setTemperatureOffset(float offsetC);
    
private:
    SensirionI2CScd4x _scd40;
    
    // Data cache
    uint16_t _co2;
    float _temperature;
    float _humidity;
    bool _dataValid;
    
    // State management
    unsigned long _lastMeasurementTime;
    bool _measurementStarted;
    
    static constexpr unsigned long MEASUREMENT_INTERVAL = 5000; // 5 seconds
    static constexpr unsigned long MEASUREMENT_DURATION = 5000; // SCD40 takes ~5s
};

#endif // CO2_SENSOR_HPP
