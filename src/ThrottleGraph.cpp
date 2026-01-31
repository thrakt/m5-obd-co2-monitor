#include "ThrottleGraph.hpp"
#include "Display.hpp"
#include <algorithm>

// RGB565色定数の定義（M5GFXから独立）
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

  // スプライトを作成（チラツキ防止）
  if (_canvas) {
    _canvas->createCanvas(_w, _h);
  }

  // 履歴バッファを確保
  _throttleHistory.clear();
  _throttleHistory.reserve(_historySize);
}

void ThrottleGraph::addData(float throttlePercent) {
  // 範囲を0-100%に制限
  throttlePercent = std::min(std::max(throttlePercent, 0.0f), 100.0f);

  // データを追加
  _throttleHistory.push_back(throttlePercent);

  // 履歴サイズを超えたら古いデータを削除
  if (_throttleHistory.size() > _historySize) {
    _throttleHistory.erase(_throttleHistory.begin());
  }
}

void ThrottleGraph::draw() {
  if (!_canvas || _throttleHistory.empty()) {
    return;
  }

  // スプライトに描画（背景クリア）
  _canvas->fillCanvas(TFT_BLACK);

  // グリッドラインを描画 (25%, 50%, 75%)
  _canvas->drawLine(0, _h * 3 / 4, _w, _h * 3 / 4, 0x2104); // 25%
  _canvas->drawLine(0, _h / 2, _w, _h / 2, 0x4208);         // 50%
  _canvas->drawLine(0, _h / 4, _w, _h / 4, 0x2104);         // 75%

  // 底辺のベースライン
  _canvas->drawLine(0, _h - 1, _w, _h - 1, TFT_DARKGREY);

  // データポイント数に応じた描画
  int dataSize = _throttleHistory.size();
  if (dataSize < 2) {
    // データが不足している場合は現在値のみ表示
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

  // データを右詰めで描画するため、開始位置を計算
  int startX = 0;
  if (dataSize < _w) {
    startX = _w - dataSize;
  }

  // 折れ線グラフとして描画
  for (int i = 1; i < dataSize; i++) {
    float val1 = _throttleHistory[i - 1];
    float val2 = _throttleHistory[i];

    // Y座標を計算（上が100%、下が0%）
    int y1 = _h - (int)(val1 * _h / 100.0f);
    int y2 = _h - (int)(val2 * _h / 100.0f);

    // X座標を計算
    int x1, x2;
    if (dataSize > _w) {
      // データポイントが幅より多い場合は圧縮
      x1 = (i - 1) * _w / dataSize;
      x2 = i * _w / dataSize;
    } else {
      x1 = startX + i - 1;
      x2 = startX + i;
    }

    // 色を決定（最新値に基づく）
    uint16_t color = getThrottleColor(val2);

    // 線を描画
    _canvas->drawLine(x1, y1, x2, y2, color);

    // 線を太くするために上下に1ピクセルずつ追加
    if (y1 > 0)
      _canvas->drawLine(x1, y1 - 1, x2, y2 - 1, color);
    if (y1 < _h - 1)
      _canvas->drawLine(x1, y1 + 1, x2, y2 + 1, color);
  }

  // 現在値を右端に表示
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

  // スプライトをディスプレイに転送（一度の転送でチラツキ防止）
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
  // アクセル開度に応じた色
  // 0-30%: 青系
  // 30-60%: 緑系
  // 60-100%: オレンジ～赤系

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
