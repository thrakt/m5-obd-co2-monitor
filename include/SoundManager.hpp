#ifndef SOUND_MANAGER_HPP
#define SOUND_MANAGER_HPP

#include <Arduino.h>
#include <M5Unified.h>

class SoundManager {
public:
    SoundManager();
    void begin();
    
    // Notification sounds
    void playPon();       // High chime (heater ready)
    void playPonPon();    // High->Low chime (cooling detected)
    void playBeee();      // Lock-on warning (CO2 alert)
    
    // State management
    void update();
    bool isPlaying();

private:
    // Tone sequence playback
    const uint16_t* _frequencies;
    const uint16_t* _durations;
    volatile uint8_t _sequenceCount;
    volatile uint8_t _sequenceIndex;
    unsigned long _nextToneTime;
    volatile bool _isPlayingSequence;

    void playToneSequence(const uint16_t* frequencies, const uint16_t* durations, uint8_t count);
    void stopSound();
};

#endif // SOUND_MANAGER_HPP
