#ifndef LED_MANAGER_HPP
#define LED_MANAGER_HPP

#include <Arduino.h>
#include <FastLED.h>

class LedManager {
public:
    LedManager();
    void begin();
    
    // Update breathing effect (non-blocking)
    void update(uint16_t co2Level);
    
    void clear();
    
private:
    static constexpr uint8_t LED_PIN = 5;
    static constexpr uint8_t NUM_LEDS = 10;
    static constexpr uint16_t BREATHING_CYCLE_MS = 3000;
    
    CRGB _leds[NUM_LEDS];
    
    // Breathing effect state
    unsigned long _breathingStartTime;
    float _breathingPhase; // 0.0 to 1.0
    
    // CO2 thresholds
    static constexpr uint16_t CO2_THRESHOLD_OFF = 1250;
    static constexpr uint16_t CO2_THRESHOLD_RED = 1500;
    
    // Helper methods
    uint8_t calculateBrightness();
    CRGB getColorForCo2(uint16_t co2Level);
};

#endif // LED_MANAGER_HPP
