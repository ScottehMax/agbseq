#ifndef AGBSEQ_AUDIO_H
#define AGBSEQ_AUDIO_H

#include <tonc.h>

#include "instruments.h"

typedef enum AudioChannel {
    AUDIO_CH_SQUARE1 = 0,
    AUDIO_CH_SQUARE2 = 1,
    AUDIO_CH_WAVE = 2,
    AUDIO_CH_NOISE = 3,
    AUDIO_CH_COUNT = 4
} AudioChannel;

void audio_init(void);
void audio_silence(void);
void audio_trigger_note(AudioChannel channel, u8 note, const Instrument *instrument);
void audio_stop_channel(AudioChannel channel);

#endif

