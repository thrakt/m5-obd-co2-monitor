#include "SoundManager.hpp"

// --- Sound Definitions ---
// playPon (heater ready)
const uint16_t pon_freq[] = {659};
const uint16_t pon_dura[] = {150};

// playPonPon (cooling detected)
const uint16_t ponpon_freq[] = {659, 523};
const uint16_t ponpon_dura[] = {150, 150};

// playBeee (CO2 alert)
const uint16_t beee_freq[] = {800};
const uint16_t beee_dura[] = {500};

SoundManager::SoundManager()
    : _frequencies(nullptr), _durations(nullptr), _sequenceCount(0),
      _sequenceIndex(0), _nextToneTime(0), _isPlayingSequence(false) {}

void SoundManager::begin()
{
  // Configure speaker to reduce resource conflicts
  auto spk_cfg = M5.Speaker.config();
  spk_cfg.task_priority = 2;    // Lower priority to avoid conflicts
  spk_cfg.task_pinned_core = 1; // Pin to APP CPU (core 1)
  M5.Speaker.config(spk_cfg);

  M5.Speaker.begin();
  M5.Speaker.setVolume(128);
  Serial.println("Sound Manager initialized with optimized config");
}

void SoundManager::update()
{
  if (!_isPlayingSequence)
  {
    return;
  }

  if (millis() >= _nextToneTime)
  {
    if (_sequenceIndex < _sequenceCount)
    {
      // Play current tone in sequence
      uint16_t freq = _frequencies[_sequenceIndex];
      uint16_t dura = _durations[_sequenceIndex];

      Serial.printf("Playing tone: Freq=%u, Dura=%u\n", freq, dura);
      M5.Speaker.tone(freq, dura);
      // Note: removed delay() to avoid stack overflow in spk_task

      // Schedule next tone or end of sequence
      _nextToneTime = millis() + dura + 50; // Add 50ms gap
      _sequenceIndex++;
    }
    else
    {
      // Sequence finished
      stopSound();
    }
  }
}

bool SoundManager::isPlaying() { return _isPlayingSequence; }

void SoundManager::stopSound()
{
  if (_isPlayingSequence)
  {
    M5.Speaker.stop();
    _isPlayingSequence = false;
  }
  _frequencies = nullptr;
  _durations = nullptr;
  _sequenceCount = 0;
  _sequenceIndex = 0;
  Serial.println("Sound stopped.");
}

void SoundManager::playPon()
{
  Serial.println("Queueing Pon (heater ready)");
  playToneSequence(pon_freq, pon_dura, 1);
}

void SoundManager::playPonPon()
{
  Serial.println("Queueing PonPon (cooling detected)");
  playToneSequence(ponpon_freq, ponpon_dura, 2);
}

void SoundManager::playBeee()
{
  Serial.println("Queueing Beee (CO2 warning)");
  playToneSequence(beee_freq, beee_dura, 1);
}

void SoundManager::playToneSequence(const uint16_t *frequencies,
                                    const uint16_t *durations, uint8_t count)
{
  if (_isPlayingSequence)
  {
    // Don't interrupt current sound
    return;
  }

  _frequencies = frequencies;
  _durations = durations;
  _sequenceCount = count;
  _sequenceIndex = 0;
  _nextToneTime = millis(); // Start immediately
  _isPlayingSequence = true;

  Serial.printf("Starting sequence with %d tones.\n", count);
}
