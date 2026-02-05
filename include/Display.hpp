#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include <M5Unified.h>

/**
 * @brief グラフ描画用のキャンバス抽象化クラス
 *
 * M5GFXの詳細を隠蔽し、ThrottleGraphが低レベルライブラリに
 * 直接依存しないようにするためのラッパー
 */
class GraphCanvas {
public:
  GraphCanvas();

  // キャンバス管理
  void createCanvas(int16_t width, int16_t height);
  void pushToDisplay(int16_t x, int16_t y);

  // 描画プリミティブ
  void fillCanvas(uint16_t color);
  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
  void drawString(const char *text, int16_t x, int16_t y);

  // テキスト設定
  void setFont(const void *font);
  void setTextSize(uint8_t size);
  void setTextDatum(uint8_t datum);
  void setTextColor(uint16_t fgColor, uint16_t bgColor);

private:
  M5Canvas *_canvas;
  M5GFX *_display;

  friend class Display;
};

class Display {
public:
  Display();
  void begin();

  // Zone update methods
  void updateHeader(float voltage, uint16_t co2);
  void updateThrottleGauge(uint8_t throttlePercent);
  void updateCoolantTemp(int16_t tempC);
  void updateCabinEnv(float tempC, float humidity);
  void updateThrottleGraph(); // Throttle graph is drawn via GraphCanvas

  // グラフ描画用のキャンバスを取得
  GraphCanvas *getGraphCanvas();

  void clear();
  void updateDisplay();

  // Startup screen methods
  void showInitMessage(const char *message);

private:
  M5GFX *_display;
  M5Canvas _headerSprite;
  M5Canvas _throttleSprite;
  M5Canvas _coolantSprite;
  M5Canvas _cabinSprite;

  // グラフ描画用のキャンバス
  GraphCanvas _graphCanvas;
  M5Canvas _graphSprite;

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
  uint16_t getCO2Color(uint16_t co2);
};

#endif // DISPLAY_HPP
