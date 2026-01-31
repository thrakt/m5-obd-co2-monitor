#ifndef AUDIO_VISUALIZER_HPP
#define AUDIO_VISUALIZER_HPP

#include <Arduino.h>
#include <M5Unified.h>
#include <arduinoFFT.h>

class AudioVisualizer {
public:
  AudioVisualizer();
  void begin();

  // Update FFT and spectrum (non-blocking)
  void update();

  // Get spectrum data for display
  const float *getSpectrum();
  uint8_t getNumBands();

  // Resource exclusion control
  void pause();  // Stop mic input, freeze last frame
  void resume(); // Restart mic input
  bool isPaused();

private:
  static constexpr uint16_t SAMPLES = 128;
  static constexpr uint16_t SAMPLING_FREQ = 16000;
  static constexpr uint8_t NUM_BANDS = 32;

  double _vReal[SAMPLES];
  double _vImag[SAMPLES];
  float _spectrum[NUM_BANDS];
  float _spectrumSmoothed[NUM_BANDS]; // For temporal smoothing

  ArduinoFFT<double> _fft;

  bool _isPaused;
  bool _micInitialized;
  unsigned long _lastSampleTime;
  uint16_t _sampleIndex;

  // Helper methods
  void initMicrophone();
  void stopMicrophone();
  void sampleAudio();
  void computeFFT();
  void normalizeSpectrum();
};

#endif // AUDIO_VISUALIZER_HPP
