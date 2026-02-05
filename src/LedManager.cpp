#include "LedManager.hpp"

LedManager::LedManager() 
    : _breathingStartTime(0),
      _breathingPhase(0.0) {}

void LedManager::begin() {
    FastLED.addLeds<WS2812, LED_PIN, GRB>(_leds, NUM_LEDS);
    FastLED.setBrightness(255);
    clear();
    Serial.println("LED Manager initialized");
}

void LedManager::update(uint16_t co2Level) {
    // Determine color based on CO2 level
    CRGB targetColor = getColorForCo2(co2Level);
    
    // If off, just clear and return
    if (targetColor == CRGB::Black) {
        clear();
        return;
    }
    
    // Calculate breathing brightness
    unsigned long now = millis();
    unsigned long elapsed = now - _breathingStartTime;
    
    // Reset cycle if needed
    if (elapsed >= BREATHING_CYCLE_MS) {
        _breathingStartTime = now;
        elapsed = 0;
    }
    
    // Calculate phase (0.0 to 1.0)
    _breathingPhase = (float)elapsed / (float)BREATHING_CYCLE_MS;
    
    // Sine wave for smooth breathing (0 to 255)
    uint8_t brightness = calculateBrightness();
    
    // Apply to all LEDs
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        _leds[i] = targetColor;
        _leds[i].fadeToBlackBy(255 - brightness);
    }
    
    FastLED.show();
}

void LedManager::clear() {
    fill_solid(_leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
}

uint8_t LedManager::calculateBrightness() {
    // Sine wave: 0.5 + 0.5 * sin(2π * phase) gives 0 to 1
    // Scale to 30-255 for visible breathing
    float sinValue = 0.5 + 0.5 * sin(2.0 * M_PI * _breathingPhase);
    return (uint8_t)(30 + sinValue * 225);
}

CRGB LedManager::getColorForCo2(uint16_t co2Level) {
    if (co2Level < CO2_THRESHOLD_OFF) {
        return CRGB::Black;  // OFF
    } else if (co2Level < CO2_THRESHOLD_RED) {
        return CRGB::Yellow;  // YELLOW breathing
    } else {
        return CRGB::Red;     // RED breathing
    }
}
