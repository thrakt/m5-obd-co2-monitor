#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include <M5Unified.h>

class Display {
public:
  Display();
  void begin();

  // Zone update methods
  void updateHeader(float voltage, uint16_t co2);
  void updateThrottleGauge(uint8_t throttlePercent);
  void updateCoolantTemp(int16_t tempC);
  void updateCabinEnv(float tempC, float humidity);
  void updateThrottleGraph(); // Throttle graph is drawn directly by
                              // ThrottleGraph class

  void clear();
  void updateDisplay();

private:
  M5GFX *_display;
  M5Canvas _headerSprite;
  M5Canvas _throttleSprite;
  M5Canvas _coolantSprite;
  M5Canvas _cabinSprite;

  // Zone dimensions
  static constexpr int HEADER_X = 0;
  static constexpr int HEADER_Y = 0;
  static constexpr int HEADER_W = 320;
  static constexpr int HEADER_H = 30;

  static constexpr int THROTTLE_X = 0;
  static constexpr int THROTTLE_Y = 30;
  static constexpr int THROTTLE_W = 160;
  static constexpr int THROTTLE_H = 160;

  static constexpr int COOLANT_X = 160;
  static constexpr int COOLANT_Y = 30;
  static constexpr int COOLANT_W = 160;
  static constexpr int COOLANT_H = 80;

  static constexpr int CABIN_X = 160;
  static constexpr int CABIN_Y = 110;
  static constexpr int CABIN_W = 160;
  static constexpr int CABIN_H = 80;

  static constexpr int VISUALIZER_X = 0;
  static constexpr int VISUALIZER_Y = 190;
  static constexpr int VISUALIZER_W = 320;
  static constexpr int VISUALIZER_H = 50;

  // Helper methods
  uint16_t getTempColor(int16_t tempC);
  uint16_t getThrottleColor(uint8_t percent);
};

#endif // DISPLAY_HPP
