# Project Specification: m5-obd-co2-monitor

## 1. Project Overview
M5Stack CoreS3を使用した車載マルチモニターシステム。
OBD2(CAN)からの車両データ、SCD40センサーからのCO2/温湿度データを取得し、視覚的に表示する。
下部エリアにはアクセル開度の時系列グラフを表示する。
視覚（画面・LED）と聴覚（ビープ音）の両方で、暖機運転の完了や換気のタイミングをドライバーに通知する。

## 2. Hardware & Environment
- **Device:** M5Stack CoreS3 (ESP32-S3)
- **Framework:** PlatformIO / Arduino Framework
- **Ports & Connections:**
  - **Port A (I2C):** SCD40 CO2 Sensor Unit.
  - **Port C (UART):** CAN Unit. **(TX=17, RX=18)**
  - **Base Pin 5:** WS2812 LED Strip (10 LEDs).
  - **Built-in Speaker:** For notification sounds.
- **Required Libraries:**
  - M5Unified
  - FastLED@^3.6.0 (for Base LEDs)
  - ESP32-TWAI-CAN@^1.0.1 (for OBD2)
  - Sensirion I2C SCD4x@^0.4.0

## 3. Architecture & Class Design
**原則:** ヘッダファイル(`.hpp`)と実装ファイル(`.cpp`)を分離し、メインループはノンブロッキングを維持する。

### 3.1. Main Components
1.  **`Display` (Class)**
    - 画面をHeader, Throttle Gauge, Coolant Temp, Cabin Envの4つのスプライトで管理。
    - ThrottleGraph領域はGraphCanvasを介して別クラスが描画。
    - `GraphCanvas`クラス: M5GFXの抽象化レイヤーとして機能し、ThrottleGraphからのハードウェア依存を排除。
2.  **`CanManager` (Class)**
    - CAN通信管理（ESP32-TWAI-CAN使用）。
    - **PIDs:** 水温(0x05), アクセル開度(0x49), 電圧(0x42)。
    - **通信設定:** 500Kbps CAN通信、Port C (TX=17, RX=18)。
    - **リクエスト間隔:** 200ms（3つのPIDを順次ポーリング）。
    - **タイムアウト:** 1.5秒（RESPONSE_TIMEOUT * 3）でデータ無効化。
    - **デバッグ機能:** setDebugEnabled()でTWAIステータス詳細ログを有効化可能（2秒間隔）。
    - **スロットル値変換:** 生のOBD2値を0-100%に変換後、15%以下を0%、75%以上を100%に、15-75%を線形補間で0-100%に再マッピング。
3.  **`Co2Sensor` (Class)**
    - SCD40制御（I2C、Port A）。
    - 温度オフセット補正機能あり（setTemperatureOffset()）。
    - 5秒ごとにデータ更新（MEASUREMENT_INTERVAL）。
4.  **`LedManager` (Class)**
    - Bottom LED制御 (WS2812、Pin 5、10 LEDs)。
    - 呼吸エフェクト実装（3秒サイクル、30-255の輝度範囲）。
5.  **`SoundManager` (Class)**
    - スピーカー制御（内蔵スピーカー使用）。
    - トーン合成による通知音生成（非同期シーケンス再生）。
    - 3種類の通知音: Pon（暖機完了）、PonPon（冷却検出）、Beee（CO2警告）。
6.  **`ThrottleGraph` (Class)**
    - アクセル開度の履歴グラフをGraphCanvas経由で描画。
    - 設定可能な履歴サイズと更新間隔。
    - 現在の設定: 320データポイント、500ms更新間隔（160秒履歴）。
7.  **`DummyCanManager` (Class)**
    - CAN Managerのモックテスト用クラス（実装済み、テスト用途）。

## 4. UI Layout Specifications (320x240)
罫線なし。スプライトとマージンでゾーニング。

### Zone 1: Header (320x30)
- **Position:** X=0, Y=0, W=320, H=30
- **Background:** Black
- **Left:** Battery Voltage (e.g., "12.4V"). Red if < 12.0V, White otherwise.
- **Center:** CO2 Value (e.g., "850 ppm"). White text.
- **Font:** FreeSansBold12pt7b

### Zone 2: Left Main (160x160) - Throttle Gauge
- **Position:** X=0, Y=30, W=160, H=160
- **UI:** Circular Gauge with Arc.
  - Center: Percentage text (e.g., "35%").  
  - Label: "Throttle" (below center).
  - Arc Color: 
    - < 33%: Blue (TFT_BLUE)
    - 33-66%: Green (TFT_GREEN)
    - > 66%: Orange (TFT_ORANGE)
  - Arc: 270度、135度から開始。
- **Font:** FreeSansBold12pt7b (value), FreeSansBold9pt7b (label)

### Zone 3: Right Top (160x80) - Coolant Temp
- **Position:** X=160, Y=30, W=160, H=80
- **Label:** "Coolant" (Small, at Y=15)
- **Value:** Digital Number (Large, center+10) + " C"
- **Color Logic:**
  - **< 60°C:** BLUE
  - **60°C <= T < 75°C:** YELLOW
  - **75°C <= T < 100°C:** ORANGE
  - **>= 100°C:** RED
- **Font:** FreeSansBold9pt7b (label), FreeSansBold12pt7b (value)

### Zone 4: Right Bottom (160x80) - Cabin Env
- **Position:** X=160, Y=110, W=160, H=80
- **Label:** "Cabin" (Small, at Y=15)
- **Value:** Single line (e.g., "24.5C / 60%", center+10)
- **Font:** FreeSansBold9pt7b (label), FreeSansBold12pt7b (value)
- **Color:** White

### Zone 5: Footer (320x50) - Throttle Graph
- **Position:** X=0, Y=190, W=320, H=50
- **Content:** Throttle Position History Graph (Line Graph with Grid).
- **Features:**
  - 時系列折れ線グラフ（右詰め、左がold、右がnew）。
  - グリッドライン表示（25%, 50%, 75%）。
  - 現在値を右上に表示（例: "35%"）。
  - カラーコード:
    - < 30%: Cyan (TFT_CYAN)
    - 30-60%: Green (TFT_GREEN)
    - 60-80%: Orange (TFT_ORANGE)
    - >= 80%: Red (TFT_RED)
- **Implementation:** ThrottleGraphクラスがGraphCanvas経由でスプライトに描画し、ディスプレイに転送。

## 5. Functional Requirements & Logic

### 5.1. Display Update Strategy
- 各ゾーンは専用スプライトで描画（Header, Throttle Gauge, Coolant, Cabin）。
- ThrottleGraphはGraphCanvas経由で独自のスプライトで描画し、直接ディスプレイに転送。
- メインループでは33ms間隔でスプライトをディスプレイにpush（約30fps）。
- ThrottleGraphは500ms間隔でデータ追加と描画実行。

### 5.2. LED Notification (Breathing Effect)
- **Interval:** 3秒サイクル（正弦波による呼吸エフェクト、BREATHING_CYCLE_MS = 3000）。
- **Logic:**
  - **CO2 < 1250 ppm:** OFF (Black).
  - **1250 <= CO2 < 1500:** YELLOW breathing.
  - **CO2 >= 1500:** RED breathing.
- **Brightness Range:** 30-255（正弦波で計算）。

### 5.3. Audio Notification (Tone Synthesis)
非同期トーンシーケンス再生により、ノンブロッキング動作を実現。

1.  **Heater Logic (Cyclic Flag):**
    - Variable: `isHeaterReady` (initially `false`).
    - **Thresholds:** HEATER_READY_TEMP = 75, HEATER_COLD_TEMP = 60
    - **Rule A (Enter):** If `!isHeaterReady` AND `Temp >= 75°C`:
      - Set `isHeaterReady = true`.
      - Play "Pon" (High chime, 659Hz, 150ms).
    - **Rule B (Exit):** If `isHeaterReady` AND `Temp <= 60°C`:
      - Set `isHeaterReady = false`.
      - Play "Pon-Pon" (High->Low chimes, 659Hz/523Hz, 150ms each).
2.  **CO2 Warning:**
    - **Thresholds:** CO2_WARNING_THRESHOLD = 1500, CO2_WARNING_DISABLE_THRESHOLD = 1250
    - **Trigger:** CO2 >= 1500 ppm (初回検出時のみ再生).
    - **Sound:** "Beee" (800Hz, 500ms).
    - **Logic:** 
      - CO2 >= 1500 ppmで`isCo2WarningActive = true`となり、1回だけBeee再生。
      - CO2 < 1250 ppmで`isCo2WarningActive = false`となり、状態解除。

**Sound Playback Implementation:**
- `SoundManager`は非同期トーンシーケンス方式を採用。
- `playToneSequence()`でシーケンスをキューイングし、`update()`内で順次再生。
- 再生中は`isPlaying()`がtrueを返す。

### 5.4. Startup Behavior
- **起動時サウンド:** setup()内で`soundManager.playPon()`を再生し、完了を待機（起動完了通知）。
- **Grace Period:** 起動音再生完了後、10秒間のdelay()で待機。この間に表示は"Waiting for sensors..."メッセージを表示。
- **実装:** setup()の最後にdelay(10000)を実行することで、警告システムが起動しないようにする単純な実装。

### 5.5. Defensive Coding Constraints
1.  **Startup Grace Period:**
    - For the first **10 seconds** after boot (implemented as delay(10000) at end of setup()), suppress ALL warnings to avoid noise during engine cranking and system stabilization.
2.  **Resource Management:**
    - `SoundManager`はM5Unifiedのスピーカー機能を使用。
    - **現在の実装:** AudioVisualizerクラスは存在していない（過去の実装から削除済み）。
3.  **Non-blocking Update:**
    - 全てのコンポーネントは`update()`メソッドにより非同期更新。
    - `delay()`の使用は最小限（setup内の10秒待機と、メインループに10msのみ）。

## 6. Display Refresh Strategy
- **Component Update:** 毎ループでcanManager, co2Sensor, soundManagerをupdate()。
- **Display Zone Update:** 毎ループで各ゾーンのスプライトを更新。
- **Throttle Graph:** 500ms間隔でデータ追加と描画（GRAPH_UPDATE_INTERVAL_MS = 500）。
- **Display Push:** 33ms間隔（DISPLAY_UPDATE_INTERVAL_MS = 33、約30fps）で全スプライトをディスプレイにpush。
- **Performance Optimization:** スプライト使用により、チラツキ防止と高速描画を実現。

## 7. Implementation Details

### 7.1. CAN Manager Details
- **Pin Configuration:** TX=17 (const int CAN_TX_PIN), RX=18 (const int CAN_RX_PIN)
- **PIDs:**
  - PID_COOLANT_TEMP = 0x05
  - PID_THROTTLE_POS = 0x49
  - PID_CONTROL_MODULE_VOLTAGE = 0x42
- **OBD2 IDs:**
  - OBD2_REQUEST_ID = 0x7DF
  - OBD2_RESPONSE_ID = 0x7E8 (also accepts 0x7E8-0x7EF range)
- **Timing:**
  - REQUEST_INTERVAL = 200ms
  - RESPONSE_TIMEOUT = 500ms
  - DEBUG_INTERVAL = 2000ms
- **Throttle Conversion:**
  1. Raw OBD2 value converted to 0-100% using formula: `(data[3] * 100) / 255`
  2. Re-map to compensate for sensor range:
     - rawThrottle <= 15: output 0%
     - rawThrottle >= 75: output 100%
     - 15 < rawThrottle < 75: linear interpolation `((rawThrottle - 15) * 100) / 60`

### 7.2. Throttle Graph Details
- **History Size:** 320 data points (GRAPH_HISTORY_SIZE = 320)
- **Update Interval:** 500ms (GRAPH_UPDATE_INTERVAL_MS = 500)
- **Total History Duration:** 160 seconds (calculated as historySize * updateInterval / 1000)
- **Graph Area:** X=0, Y=190, W=320, H=50

### 7.3. CO2 Sensor Details
- **Measurement Interval:** 5000ms (MEASUREMENT_INTERVAL = 5000)
- **Measurement Duration:** 5000ms (MEASUREMENT_DURATION = 5000, SCD40 takes ~5s per reading)

### 7.4. LED Manager Details
- **LED Pin:** 5 (LED_PIN = 5)
- **LED Count:** 10 LEDs (NUM_LEDS = 10)
- **Breathing Cycle:** 3000ms (BREATHING_CYCLE_MS = 3000)
- **Thresholds:**
  - CO2_THRESHOLD_OFF = 1250 ppm
  - CO2_THRESHOLD_RED = 1500 ppm

## 8. Current Status & Notes
- **ThrottleGraph:** アクセル開度の可視化を提供。GraphCanvasを使用することで、M5GFXへの直接依存を回避。
- **CAN Debug:** デバッグフラグ（setDebugEnabled(true)）により、詳細なTWAIステータスロギングを実装（2秒間隔でステータス情報を出力）。
- **Temperature Offset:** Co2SensorはsetTemperatureOffset()により、キャリブレーション可能。
- **DummyCanManager:** テスト用のダミーCANマネージャーが実装されている。