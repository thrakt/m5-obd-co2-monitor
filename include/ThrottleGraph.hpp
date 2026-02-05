#ifndef THROTTLE_GRAPH_HPP
#define THROTTLE_GRAPH_HPP

#include <cstdint>
#include <vector>

// Forward declaration
class GraphCanvas;

/**
 * @brief Class for displaying throttle position history graph
 *
 * As a replacement for AudioVisualizer, draws throttle position time-series
 * graph in the bottom area without using audio resources. Uses GraphCanvas to
 * eliminate direct dependency on M5GFX and improve maintainability.
 */
class ThrottleGraph {
public:
  ThrottleGraph();

  /**
   * @brief Initialize
   * @param canvas Canvas for graph drawing (obtained via Display)
   * @param x X coordinate of graph area top-left
   * @param y Y coordinate of graph area top-left
   * @param w Graph width
   * @param h Graph height
   * @param historySize Number of history data points (default: 160)
   * @param updateIntervalMs Data update interval in milliseconds (default: 100)
   */
  void begin(GraphCanvas *canvas, int16_t x, int16_t y, int16_t w, int16_t h,
             uint16_t historySize = 160, uint16_t updateIntervalMs = 100);

  /**
   * @brief Add throttle position data
   * @param throttlePercent Throttle position (0-100%)
   */
  void addData(float throttlePercent);

  /**
   * @brief Draw graph and transfer to display
   */
  void draw();

  /**
   * @brief Clear graph
   */
  void clear();

  /**
   * @brief Get total history duration (seconds)
   * @return History duration (seconds)
   */
  float getHistoryDurationSeconds() const;

private:
  GraphCanvas *_canvas;
  int16_t _x, _y, _w, _h;
  uint16_t _historySize;
  uint16_t _updateIntervalMs;
  std::vector<float> _throttleHistory;

  /**
   * @brief Get color according to throttle position
   * @param throttlePercent Throttle position (0-100%)
   * @return RGB565 color
   */
  uint16_t getThrottleColor(float throttlePercent);
};

#endif // THROTTLE_GRAPH_HPP
