#ifndef AGBSEQ_SEQUENCER_H
#define AGBSEQ_SEQUENCER_H

#include <tonc.h>

#include "song.h"

typedef struct Sequencer {
    const Song *song;
    bool playing;
    u8 order_index;
    u8 row;
    u8 frames_until_row;
    u8 frames_per_row;
} Sequencer;

void sequencer_init(Sequencer *sequencer, const Song *song);
void sequencer_toggle_play(Sequencer *sequencer);
void sequencer_stop(Sequencer *sequencer);
void sequencer_update(Sequencer *sequencer);
void sequencer_set_tempo(Sequencer *sequencer, u8 frames_per_row);

#endif

