#include "ThrottleGraph.hpp"
#include "Display.hpp"
#include <algorithm>

// RGB565 color constants definition (independent from M5GFX)）
#define TFT_BLACK 0x0000
#define TFT_DARKGREY 0x4208
#define TFT_CYAN 0x07FF
#define TFT_GREEN 0x07E0
#define TFT_ORANGE 0xFD20
#define TFT_RED 0xF800
#define top_right 7 // M5GFX datum constant

ThrottleGraph::ThrottleGraph()
    : _canvas(nullptr), _x(0), _y(0), _w(0), _h(0), _historySize(160),
      _updateIntervalMs(100) {}

void ThrottleGraph::begin(GraphCanvas *canvas, int16_t x, int16_t y, int16_t w,
                          int16_t h, uint16_t historySize,
                          uint16_t updateIntervalMs) {
  _canvas = canvas;
  _x = x;
  _y = y;
  _w = w;
  _h = h;
  _historySize = historySize;
  _updateIntervalMs = updateIntervalMs;

  // Create sprite (prevents flickering)）
  if (_canvas) {
    _canvas->createCanvas(_w, _h);
  }

  // Allocate history buffer
  _throttleHistory.clear();
  _throttleHistory.reserve(_historySize);
}

void ThrottleGraph::addData(float throttlePercent) {
  // Limit range to 0-100%
  throttlePercent = std::min(std::max(throttlePercent, 0.0f), 100.0f);

  // Add data
  _throttleHistory.push_back(throttlePercent);

  // Remove old data when exceeds history size
  if (_throttleHistory.size() > _historySize) {
    _throttleHistory.erase(_throttleHistory.begin());
  }
}

void ThrottleGraph::draw() {
  if (!_canvas || _throttleHistory.empty()) {
    return;
  }

  // Draw to sprite (clear background)）
  _canvas->fillCanvas(TFT_BLACK);

  // Draw grid lines (25%, 50%, 75%)
  _canvas->drawLine(0, _h * 3 / 4, _w, _h * 3 / 4, 0x2104); // 25%
  _canvas->drawLine(0, _h / 2, _w, _h / 2, 0x4208);         // 50%
  _canvas->drawLine(0, _h / 4, _w, _h / 4, 0x2104);         // 75%

  // Baseline at bottom
  _canvas->drawLine(0, _h - 1, _w, _h - 1, TFT_DARKGREY);

  // Draw according to number of data points
  int dataSize = _throttleHistory.size();
  if (dataSize < 2) {
    // Display only current value when data is insufficient
    if (dataSize == 1) {
      float currentValue = _throttleHistory[0];
      char buf[16];
      snprintf(buf, sizeof(buf), "%.0f%%", currentValue);
      _canvas->setFont(&fonts::FreeSansBold12pt7b);
      _canvas->setTextSize(1);
      _canvas->setTextDatum(top_right);
      _canvas->setTextColor(getThrottleColor(currentValue), TFT_BLACK);
      _canvas->drawString(buf, _w - 5, 5);
    }
    _canvas->pushToDisplay(_x, _y);
    return;
  }

  // Calculate start position for right-aligned drawing
  int startX = 0;
  if (dataSize < _w) {
    startX = _w - dataSize;
  }

  // Draw as line graph
  for (int i = 1; i < dataSize; i++) {
    float val1 = _throttleHistory[i - 1];
    float val2 = _throttleHistory[i];

    // Calculate Y coordinates (top is 100%, bottom is 0%)）
    int y1 = _h - (int)(val1 * _h / 100.0f);
    int y2 = _h - (int)(val2 * _h / 100.0f);

    // Calculate X coordinates
    int x1, x2;
    if (dataSize > _w) {
      // Compress when data points exceed width
      x1 = (i - 1) * _w / dataSize;
      x2 = i * _w / dataSize;
    } else {
      x1 = startX + i - 1;
      x2 = startX + i;
    }

    // Determine color (based on latest value)）
    uint16_t color = getThrottleColor(val2);

    // Draw line
    _canvas->drawLine(x1, y1, x2, y2, color);

    // Add 1 pixel above and below to thicken the line
    if (y1 > 0)
      _canvas->drawLine(x1, y1 - 1, x2, y2 - 1, color);
    if (y1 < _h - 1)
      _canvas->drawLine(x1, y1 + 1, x2, y2 + 1, color);
  }

  // Display current value at right edge
  if (!_throttleHistory.empty()) {
    float currentValue = _throttleHistory.back();
    char buf[16];
    snprintf(buf, sizeof(buf), "%.0f%%", currentValue);

    _canvas->setFont(&fonts::FreeSansBold12pt7b);
    _canvas->setTextSize(1);
    _canvas->setTextDatum(top_right);
    _canvas->setTextColor(getThrottleColor(currentValue), TFT_BLACK);
    _canvas->drawString(buf, _w - 25, 10);
  }

  // Transfer sprite to display (prevents flickering with single transfer)）
  _canvas->pushToDisplay(_x, _y);
}

void ThrottleGraph::clear() {
  _throttleHistory.clear();
  if (_canvas) {
    _canvas->fillCanvas(TFT_BLACK);
    _canvas->pushToDisplay(_x, _y);
  }
}

float ThrottleGraph::getHistoryDurationSeconds() const {
  return (_historySize * _updateIntervalMs) / 1000.0f;
}

uint16_t ThrottleGraph::getThrottleColor(float throttlePercent) {
  // Color according to throttle position
  // 0-30%: Blue
  // 30-60%: Green
  // 60-100%: Orange to Red

  if (throttlePercent < 30.0f) {
    return TFT_CYAN;
  } else if (throttlePercent < 60.0f) {
    return TFT_GREEN;
  } else if (throttlePercent < 80.0f) {
    return TFT_ORANGE;
  } else {
    return TFT_RED;
  }
}
