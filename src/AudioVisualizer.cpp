#include "AudioVisualizer.hpp"

AudioVisualizer::AudioVisualizer()
    : _fft(ArduinoFFT<double>(_vReal, _vImag, SAMPLES, SAMPLING_FREQ)),
      _isPaused(false), _micInitialized(false), _lastSampleTime(0),
      _sampleIndex(0) {
  // Initialize arrays
  for (uint16_t i = 0; i < SAMPLES; i++) {
    _vReal[i] = 0.0;
    _vImag[i] = 0.0;
  }
  for (uint8_t i = 0; i < NUM_BANDS; i++) {
    _spectrum[i] = 0.0;
    _spectrumSmoothed[i] = 0.0;
  }
}

void AudioVisualizer::begin() {
  initMicrophone();
  Serial.println("Audio Visualizer initialized");
}

void AudioVisualizer::update() {
  if (_isPaused || !_micInitialized) {
    return; // Keep last frame when paused
  }

  // Sample audio at regular intervals
  sampleAudio();

  // When we have enough samples, compute FFT
  if (_sampleIndex >= SAMPLES) {
    computeFFT();
    normalizeSpectrum();
    _sampleIndex = 0; // Reset for next batch
  }
}

const float *AudioVisualizer::getSpectrum() { return _spectrum; }

uint8_t AudioVisualizer::getNumBands() { return NUM_BANDS; }

void AudioVisualizer::pause() {
  if (_isPaused)
    return;

  Serial.println("Audio Visualizer paused - stopping microphone");
  stopMicrophone(); // Release I2S resources
  _isPaused = true;
  // Spectrum data is frozen (last frame maintained)
}

void AudioVisualizer::resume() {
  if (!_isPaused)
    return;

  Serial.println("Audio Visualizer resumed - restarting microphone");
  initMicrophone(); // Re-acquire I2S resources
  _isPaused = false;
}

bool AudioVisualizer::isPaused() { return _isPaused; }

void AudioVisualizer::initMicrophone() {
  if (_micInitialized)
    return;

  // Initialize built-in microphone via M5Unified
  auto mic_cfg = M5.Mic.config();
  mic_cfg.sample_rate = SAMPLING_FREQ; // 16000 Hz
  mic_cfg.stereo = false;              // Mono
  mic_cfg.over_sampling = 1;
  M5.Mic.config(mic_cfg);

  if (!M5.Mic.begin()) {
    Serial.println("Microphone initialization failed");
    return;
  }

  _micInitialized = true;
  _sampleIndex = 0;
  Serial.printf("Microphone initialized successfully at %d Hz\n",
                SAMPLING_FREQ);
}

void AudioVisualizer::stopMicrophone() {
  if (!_micInitialized)
    return;

  M5.Mic.end();
  _micInitialized = false;

  // Wait to ensure I2S resources are fully released
  // This prevents conflicts with SoundManager
  delay(20);
  Serial.println("Microphone stopped and I2S released");
}

void AudioVisualizer::sampleAudio() {
  if (_sampleIndex >= SAMPLES)
    return;

  // Read microphone data without blocking
  // Attempt multiple reads per update to speed up sample collection
  if (M5.Mic.isEnabled()) {
    static int16_t audio_buffer[256];

    // Try up to 10 times or until buffer is full
    for (int attempt = 0; attempt < 10 && _sampleIndex < SAMPLES; attempt++) {
      size_t samples_to_read = min(256, SAMPLES - _sampleIndex);
      size_t samples_read = M5.Mic.record(audio_buffer, samples_to_read);

      if (samples_read > 0) {
        // Copy to FFT buffer with normalization
        // Normalize int16_t range (-32768 to 32767) to -1.0 to 1.0
        for (size_t i = 0; i < samples_read && _sampleIndex < SAMPLES;
             i++, _sampleIndex++) {
          _vReal[_sampleIndex] = (double)audio_buffer[i] / 32768.0;
          _vImag[_sampleIndex] = 0.0;
        }
      } else {
        // No more data available
        break;
      }
    }
  }
}

void AudioVisualizer::computeFFT() {
  // Apply windowing
  _fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);

  // Compute FFT
  _fft.compute(FFTDirection::Forward);

  // Compute magnitudes
  _fft.complexToMagnitude();
}

void AudioVisualizer::normalizeSpectrum() {
  // Map FFT bins to spectrum bands
  uint16_t samplesPerBand = (SAMPLES / 2) / NUM_BANDS;

  // Smoothing factor for temporal smoothing (0.0-1.0)
  const float SMOOTHING_FACTOR = 0.3; // Fast response

  // Reference level for normalization (adjust based on typical audio levels)
  // This is the level that should map to ~0.5 on the display
  const float REFERENCE_LEVEL = 0.1;

  // Gain to apply after normalization
  const float GAIN_FACTOR = 0.8; // Conservative to avoid clipping

  // Process each band
  for (uint8_t i = 0; i < NUM_BANDS; i++) {
    float sum = 0.0;
    uint16_t startBin = i * samplesPerBand;
    uint16_t endBin = startBin + samplesPerBand;

    // Average the bins in this band
    for (uint16_t j = startBin; j < endBin && j < SAMPLES / 2; j++) {
      sum += _vReal[j];
    }

    float average = sum / samplesPerBand;

    // Normalize against reference level (not max)
    float normalized = (average / REFERENCE_LEVEL) * GAIN_FACTOR;

    // Clamp to valid range
    if (normalized > 1.0)
      normalized = 1.0;
    if (normalized < 0.0)
      normalized = 0.0;

    // Apply temporal smoothing
    _spectrumSmoothed[i] = _spectrumSmoothed[i] * SMOOTHING_FACTOR +
                           normalized * (1.0 - SMOOTHING_FACTOR);

    // Use smoothed value for output
    _spectrum[i] = _spectrumSmoothed[i];
  }
}
