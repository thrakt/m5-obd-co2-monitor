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
  - **Port C (UART):** CAN Unit. **(TX=18, RX=17)**
  - **Base Pin 5:** WS2812 LED Strip (10 LEDs).
  - **Built-in Speaker:** For notification sounds.
  - **Built-in Mic:** For Audio Visualizer (currently not actively used in UI, but class exists for future use).
- **Required Libraries:**
  - M5Unified
  - FastLED (for Base LEDs)
  - arduinoFFT (for Audio Visualizer)
  - ESP32-TWAI-CAN (for OBD2)
  - Sensirion I2C SCD4x

## 3. Architecture & Class Design
**原則:** ヘッダファイル(`.hpp`)と実装ファイル(`.cpp`)を分離し、メインループはノンブロッキングを維持する。

### 3.1. Main Components
1.  **`Display` (Class)**
    - 画面をHeader, Throttle Gauge, Coolant Temp, Cabin Envの4つのスプライトで管理。
    - ThrottleGraph領域は別クラスが直接描画するため、スプライトなし。
2.  **`CanManager` (Class)**
    - CAN通信管理（ESP32-TWAI-CAN使用）。
    - PID: 水温(0x05), アクセル開度(0x11), 電圧(0x42)。
    - 500Kbps CAN通信、Port C (TX=18, RX=17)。
3.  **`Co2Sensor` (Class)**
    - SCD40制御（I2C、Port A）。
    - 温度オフセット補正機能あり。
    - 5秒ごとにデータ更新。
4.  **`LedManager` (Class)**
    - Bottom LED制御 (WS2812、Pin 5、10 LEDs)。
    - 呼吸エフェクト実装（3秒サイクル）。
5.  **`SoundManager` (Class)**
    - スピーカー制御（内蔵スピーカー使用）。
    - トーン合成による通知音生成（非同期シーケンス再生）。
    - 3種類の通知音: Pon（暖機完了）、PonPon（冷却検出）、Beee（CO2警告）。
6.  **`ThrottleGraph` (Class)**
    - アクセル開度の履歴グラフをスプライト使用で描画。
    - 設定可能な履歴サイズと更新間隔。
    - 現在の設定: 320データポイント、500ms更新間隔（160秒履歴）。
7.  **`AudioVisualizer` (Class)**
    - マイク入力FFT解析とスペアナ機能を提供。
    - **排他制御:** pause/resume機能により、音声再生時はマイク入力を停止する。
    - **現在の使用状況:** 実装されているが、UIには現在組み込まれていない（将来の拡張用）。

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
- **Implementation:** ThrottleGraphクラスがスプライト経由で直接ディスプレイに描画。

## 5. Functional Requirements & Logic

### 5.1. Display Update Strategy
- 各ゾーンは専用スプライトで描画（Header, Throttle Gauge, Coolant, Cabin）。
- ThrottleGraphは独自のスプライトで描画し、直接ディスプレイに転送。
- メインループでは33ms間隔でスプライトをディスプレイにpush（約30fps）。
- ThrottleGraphは500ms間隔でデータ追加と描画実行。

### 5.2. LED Notification (Breathing Effect)
- **Interval:** ~3秒サイクル（正弦波による呼吸エフェクト）。
- **Logic:**
  - **CO2 < 1250 ppm:** OFF (Black).
  - **1250 <= CO2 < 1500:** YELLOW breathing.
  - **CO2 >= 1500:** RED breathing.
- **Brightness Range:** 30-255（正弦波）。

### 5.3. Audio Notification (Tone Synthesis)
非同期トーンシーケンス再生により、ノンブロッキング動作を実現。

1.  **Heater Logic (Cyclic Flag):**
    - Variable: `isHeaterReady` (initially `false`).
    - **Rule A (Enter):** If `!isHeaterReady` AND `Temp >= 75°C`:
      - Set `isHeaterReady = true`.
      - Play "Pon" (High chime, 659Hz, 150ms).
    - **Rule B (Exit):** If `isHeaterReady` AND `Temp <= 60°C`:
      - Set `isHeaterReady = false`.
      - Play "Pon-Pon" (High->Low chimes, 659Hz/523Hz, 150ms each).
2.  **CO2 Warning:**
    - **Trigger:** CO2 >= 1500 ppm (初回検出時のみ再生).
    - **Sound:** "Beee" (800Hz, 500ms).
    - **Logic:** CO2 >= 1500でwarning状態に入り、1回だけBeee再生。CO2 < 1500で状態解除。

**Sound Playback Implementation:**
- `SoundManager`は非同期トーンシーケンス方式を採用。
- `playToneSequence()`でシーケンスをキューイングし、`update()`内で順次再生。
- 再生中は`isPlaying()`がtrueを返す。

### 5.4. Startup Behavior
- **起動時サウンド:** setup()内で`soundManager.playPon()`を再生（起動完了通知）。
- **Grace Period:** 起動後10秒間は全警告を抑制（電圧警告、LED警告、ビープ音）。

### 5.5. Defensive Coding Constraints
1.  **Startup Grace Period:**
    - For the first **10 seconds** after boot, suppress ALL warnings (Voltage Red, LED Red, Beep Sounds) to avoid noise during engine cranking and system stabilization.
2.  **Resource Management:**
    - `SoundManager`はスピーカー設定を最適化（task_priority=2, task_pinned_core=1）。
    - `AudioVisualizer`はpause/resume機能により、I2Sリソース競合を回避。
    - **現在の実装:** AudioVisualizerは実装されているが、メインループには組み込まれていない。
3.  **Non-blocking Update:**
    - 全てのコンポーネントは`update()`メソッドにより非同期更新。
    - `delay()`の使用を最小化（メインループに10msのみ）。

## 6. Display Refresh Strategy
- **Component Update:** 毎ループでcanManager, co2Sensor, soundManagerをupdate()。
- **Throttle Graph:** 500ms間隔でデータ追加と描画（main.cppで管理）。
- **Display Push:** 33ms間隔（約30fps）で全スプライトをディスプレイにpush。
- **Performance Optimization:** スプライト使用により、チラツキ防止と高速描画を実現。

## 7. Current Status & Notes
- **AudioVisualizer:** 実装済みだが、現在UIには組み込まれていない。将来的にThrottleGraphと切り替え可能にする予定。
- **ThrottleGraph:** AudioVisualizerの代替として実装。音声リソースを使用せず、アクセル開度の可視化を提供。
- **CAN Debug:** 詳細なステータスロギングを実装（10秒間隔でデバッグ情報を出力）。
- **Temperature Offset:** Co2SensorはsetTemperatureOffset()により、キャリブレーション可能。