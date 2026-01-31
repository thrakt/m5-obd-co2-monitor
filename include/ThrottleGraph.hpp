#ifndef THROTTLE_GRAPH_HPP
#define THROTTLE_GRAPH_HPP

#include <M5Unified.h>
#include <vector>

/**
 * @brief アクセル開度の履歴グラフを表示するクラス
 *
 * AudioVisualizerの代わりに、音声系リソースを使用せずに
 * 下部エリアにアクセル開度の時系列グラフを描画します。
 * スプライトを使用してチラツキを防止します。
 */
class ThrottleGraph {
public:
  ThrottleGraph();

  /**
   * @brief 初期化
   * @param display M5Unifiedのディスプレイポインタ
   * @param x グラフ領域の左上X座標
   * @param y グラフ領域の左上Y座標
   * @param w グラフの幅
   * @param h グラフの高さ
   * @param historySize 履歴データポイント数（デフォルト: 160）
   * @param updateIntervalMs データ更新間隔（ミリ秒、デフォルト: 100）
   */
  void begin(M5GFX *display, int16_t x, int16_t y, int16_t w, int16_t h,
             uint16_t historySize = 160, uint16_t updateIntervalMs = 100);

  /**
   * @brief アクセル開度データを追加
   * @param throttlePercent アクセル開度 (0-100%)
   */
  void addData(float throttlePercent);

  /**
   * @brief グラフを描画してディスプレイに転送
   */
  void draw();

  /**
   * @brief グラフをクリア
   */
  void clear();

  /**
   * @brief 履歴の合計時間を取得（秒）
   * @return 履歴時間（秒）
   */
  float getHistoryDurationSeconds() const;

private:
  M5GFX *_display;
  M5Canvas _sprite;
  int16_t _x, _y, _w, _h;
  uint16_t _historySize;
  uint16_t _updateIntervalMs;
  std::vector<float> _throttleHistory;

  /**
   * @brief アクセル開度に応じた色を取得
   * @param throttlePercent アクセル開度 (0-100%)
   * @return RGB565色
   */
  uint16_t getThrottleColor(float throttlePercent);
};

#endif // THROTTLE_GRAPH_HPP
