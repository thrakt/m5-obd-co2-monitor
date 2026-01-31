#include "Display.hpp"

// GraphCanvas implementation
GraphCanvas::GraphCanvas() : _canvas(nullptr), _display(nullptr) {}

void GraphCanvas::createCanvas(int16_t width, int16_t height) {
  if (_canvas) {
    _canvas->createSprite(width, height);
  }
}

void GraphCanvas::pushToDisplay(int16_t x, int16_t y) {
  if (_canvas && _display) {
    _canvas->pushSprite(_display, x, y);
  }
}

void GraphCanvas::fillCanvas(uint16_t color) {
  if (_canvas) {
    _canvas->fillSprite(color);
  }
}

void GraphCanvas::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                           uint16_t color) {
  if (_canvas) {
    _canvas->drawLine(x0, y0, x1, y1, color);
  }
}

void GraphCanvas::drawString(const char *text, int16_t x, int16_t y) {
  if (_canvas) {
    _canvas->drawString(text, x, y);
  }
}

void GraphCanvas::setFont(const void *font) {
  if (_canvas) {
    _canvas->setFont((const lgfx::IFont *)font);
  }
}

void GraphCanvas::setTextSize(uint8_t size) {
  if (_canvas) {
    _canvas->setTextSize(size);
  }
}

void GraphCanvas::setTextDatum(uint8_t datum) {
  if (_canvas) {
    _canvas->setTextDatum(datum);
  }
}

void GraphCanvas::setTextColor(uint16_t fgColor, uint16_t bgColor) {
  if (_canvas) {
    _canvas->setTextColor(fgColor, bgColor);
  }
}

// Display implementation

Display::Display() : _display(&M5.Display) {}

void Display::begin() {
  _display->begin();
  _display->setRotation(1);
  _display->setBrightness(128); // Reduced brightness
  _display->fillScreen(TFT_BLACK);

  _headerSprite.createSprite(HEADER_W, HEADER_H);
  _headerSprite.setTextDatum(MC_DATUM);

  _throttleSprite.createSprite(THROTTLE_W, THROTTLE_H);
  _throttleSprite.setTextDatum(MC_DATUM);

  _coolantSprite.createSprite(COOLANT_W, COOLANT_H);
  _coolantSprite.setTextDatum(MC_DATUM);

  _cabinSprite.createSprite(CABIN_W, CABIN_H);
  _cabinSprite.setTextDatum(MC_DATUM);

  // グラフキャンバスの初期化
  _graphCanvas._canvas = &_graphSprite;
  _graphCanvas._display = _display;
}

void Display::clear() { _display->fillScreen(TFT_BLACK); }

void Display::showInitMessage(const char *message) {
  // Calculate next line position (simple scrolling effect)
  static int lineY = 40;

  if (lineY > 200) {
    // Reset if we've gone too far down
    clear();
    lineY = 40;

    // Redraw title
    _display->setTextDatum(top_center);
    _display->setTextSize(1);
    _display->setFont(&fonts::FreeSansBold12pt7b);
    _display->setTextColor(TFT_CYAN);
    _display->drawString("System Initialization", 160, 10);
    lineY = 40;
  }

  // Show first time title if this is the first message
  if (lineY == 40) {
    _display->setTextDatum(top_center);
    _display->setTextSize(1);
    _display->setFont(&fonts::FreeSansBold12pt7b);
    _display->setTextColor(TFT_CYAN);
    _display->drawString("System Initialization", 160, 10);
  }

  // Draw the message
  _display->setTextDatum(top_left);
  _display->setFont(&fonts::FreeSans9pt7b);
  _display->setTextColor(TFT_GREEN);
  _display->drawString("> ", 10, lineY);

  _display->setTextColor(TFT_WHITE);
  _display->drawString(message, 30, lineY);

  lineY += 20;
}

void Display::updateDisplay() {
  _headerSprite.pushSprite(_display, HEADER_X, HEADER_Y);
  _throttleSprite.pushSprite(_display, THROTTLE_X, THROTTLE_Y);
  _coolantSprite.pushSprite(_display, COOLANT_X, COOLANT_Y);
  _cabinSprite.pushSprite(_display, CABIN_X, CABIN_Y);
  // Note: ThrottleGraph draws directly to the display, no sprite push needed
}

void Display::updateHeader(float voltage, uint16_t co2) {
  _headerSprite.fillSprite(TFT_BLACK);

  // Use a cleaner font
  _headerSprite.setFont(&fonts::FreeSansBold12pt7b);
  _headerSprite.setTextSize(1);

  // Left: Battery Voltage
  uint16_t voltageColor = (voltage < 12.0) ? TFT_RED : TFT_WHITE;
  _headerSprite.setTextColor(voltageColor);
  char voltageStr[16];
  snprintf(voltageStr, sizeof(voltageStr), "%.1fV", voltage);
  _headerSprite.drawString(voltageStr, 40, HEADER_H / 2 + 5);

  // Center: CO2 Value
  _headerSprite.setTextColor(TFT_WHITE);
  char co2Str[16];
  snprintf(co2Str, sizeof(co2Str), "%d ppm", co2);
  _headerSprite.drawString(co2Str, HEADER_W / 2, HEADER_H / 2 + 5);
}

void Display::updateThrottleGauge(uint8_t throttlePercent) {
  _throttleSprite.fillSprite(TFT_BLACK);

  int16_t centerX = THROTTLE_W / 2;
  int16_t centerY = THROTTLE_H / 2;
  int16_t radius = 60;

  _throttleSprite.drawCircle(centerX, centerY, radius, TFT_DARKGREY);

  float angle = (throttlePercent / 100.0) * 270.0;
  float startAngle = 135.0;

  for (int i = 0; i <= angle; i++) {
    float rad = (startAngle + i) * DEG_TO_RAD;
    int16_t x = centerX + (radius - 5) * cos(rad);
    int16_t y = centerY + (radius - 5) * sin(rad);
    uint16_t color = getThrottleColor((i / 270.0) * 100);
    _throttleSprite.drawCircle(x, y, 3, color);
  }

  _throttleSprite.setFont(&fonts::FreeSansBold12pt7b);
  _throttleSprite.setTextSize(1);
  _throttleSprite.setTextColor(TFT_WHITE);
  char percentStr[8];
  snprintf(percentStr, sizeof(percentStr), "%d%%", throttlePercent);
  _throttleSprite.drawString(percentStr, centerX, centerY);

  _throttleSprite.setFont(&fonts::FreeSansBold9pt7b);
  _throttleSprite.setTextColor(TFT_LIGHTGREY);
  _throttleSprite.drawString("Throttle", centerX, centerY + 30);
}

void Display::updateCoolantTemp(int16_t tempC) {
  _coolantSprite.fillSprite(TFT_BLACK);

  int16_t centerX = COOLANT_W / 2;
  int16_t centerY = COOLANT_H / 2;

  _coolantSprite.setFont(&fonts::FreeSansBold9pt7b);
  _coolantSprite.setTextSize(1);
  _coolantSprite.setTextColor(TFT_LIGHTGREY);
  _coolantSprite.drawString("Coolant", centerX, 15);

  _coolantSprite.setFont(&fonts::FreeSansBold12pt7b);
  uint16_t tempColor = getTempColor(tempC);
  _coolantSprite.setTextSize(2);
  _coolantSprite.setTextColor(tempColor);
  char tempStr[16];
  snprintf(tempStr, sizeof(tempStr), "%d C", tempC);
  _coolantSprite.drawString(tempStr, centerX, centerY + 20);
}

void Display::updateCabinEnv(float tempC, float humidity) {
  _cabinSprite.fillSprite(TFT_BLACK);

  int16_t centerX = CABIN_W / 2;
  int16_t centerY = CABIN_H / 2;

  _cabinSprite.setFont(&fonts::FreeSansBold9pt7b);
  _cabinSprite.setTextSize(1);
  _cabinSprite.setTextColor(TFT_LIGHTGREY);
  _cabinSprite.drawString("Cabin", centerX, 15);

  _cabinSprite.setFont(&fonts::FreeSansBold12pt7b);
  _cabinSprite.setTextColor(TFT_WHITE);
  char envStr[32];
  snprintf(envStr, sizeof(envStr), "%.1fC / %.0f%%", tempC, humidity);
  _cabinSprite.drawString(envStr, centerX, centerY + 10);
}

void Display::updateThrottleGraph() {
  // ThrottleGraph draws via GraphCanvas
  // This method is kept for API consistency but does nothing
}

GraphCanvas *Display::getGraphCanvas() { return &_graphCanvas; }

uint16_t Display::getTempColor(int16_t tempC) {
  if (tempC < 60) {
    return TFT_BLUE;
  } else if (tempC < 75) {
    return TFT_YELLOW;
  } else if (tempC < 100) {
    return TFT_ORANGE;
  } else {
    return TFT_RED;
  }
}

uint16_t Display::getThrottleColor(uint8_t percent) {
  if (percent < 33) {
    return TFT_BLUE;
  } else if (percent < 66) {
    return TFT_GREEN;
  } else {
    return TFT_ORANGE;
  }
}
