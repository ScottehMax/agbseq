#include "sequencer.h"

#include "audio.h"
#include "instruments.h"

static void sequencer_process_row(Sequencer *sequencer)
{
    const u8 pattern_index = sequencer->song->order[sequencer->order_index];
    const Pattern *pattern = &sequencer->song->patterns[pattern_index];

    for(u8 track = 0; track < SONG_TRACK_COUNT; track++)
    {
        const PatternCell *cell = &pattern->cells[sequencer->row][track];

        if(cell->effect == EFFECT_SET_SPEED && cell->param != 0)
            sequencer_set_tempo(sequencer, cell->param);

        if(cell->effect == EFFECT_NOTE_CUT)
        {
            audio_stop_channel((AudioChannel)track);
            continue;
        }

        if(cell->note != SONG_EMPTY_NOTE)
            audio_trigger_note((AudioChannel)track, cell->note, instrument_get(cell->instrument));
    }

    sequencer->row++;
    if(sequencer->row >= SONG_PATTERN_ROWS)
    {
        sequencer->row = 0;
        sequencer->order_index++;
        if(sequencer->order_index >= sequencer->song->order_length)
            sequencer->order_index = 0;
    }
}

void sequencer_init(Sequencer *sequencer, const Song *song)
{
    sequencer->song = song;
    sequencer->playing = false;
    sequencer->order_index = 0;
    sequencer->row = 0;
    sequencer->frames_per_row = song->initial_frames_per_row;
    sequencer->frames_until_row = 1;
}

void sequencer_toggle_play(Sequencer *sequencer)
{
    sequencer->playing = !sequencer->playing;
    if(sequencer->playing)
        sequencer->frames_until_row = 1;
    else
        audio_silence();
}

void sequencer_stop(Sequencer *sequencer)
{
    sequencer->playing = false;
    sequencer->order_index = 0;
    sequencer->row = 0;
    sequencer->frames_until_row = sequencer->frames_per_row;
    audio_silence();
}

void sequencer_update(Sequencer *sequencer)
{
    if(!sequencer->playing)
        return;

    if(sequencer->frames_until_row > 0)
        sequencer->frames_until_row--;

    if(sequencer->frames_until_row == 0)
    {
        sequencer_process_row(sequencer);
        sequencer->frames_until_row = sequencer->frames_per_row;
    }
}

void sequencer_set_tempo(Sequencer *sequencer, u8 frames_per_row)
{
    sequencer->frames_per_row = frames_per_row;
    if(sequencer->frames_until_row > frames_per_row)
        sequencer->frames_until_row = frames_per_row;
}

