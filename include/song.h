#ifndef AGBSEQ_SONG_H
#define AGBSEQ_SONG_H

#include <tonc.h>

#define SONG_TRACK_COUNT 4
#define SONG_PATTERN_COUNT 2
#define SONG_PATTERN_ROWS 16
#define SONG_ORDER_COUNT 1
#define SONG_EMPTY_NOTE 0

typedef enum EffectCommand {
    EFFECT_NONE = 0,
    EFFECT_SET_SPEED = 1,
    EFFECT_NOTE_CUT = 2
} EffectCommand;

typedef struct PatternCell {
    u8 note;
    u8 instrument;
    u8 effect;
    u8 param;
} PatternCell;

typedef struct Pattern {
    PatternCell cells[SONG_PATTERN_ROWS][SONG_TRACK_COUNT];
} Pattern;

typedef struct Song {
    u8 initial_frames_per_row;
    u8 order_length;
    u8 order[SONG_ORDER_COUNT];
    Pattern patterns[SONG_PATTERN_COUNT];
} Song;

extern const Song g_demo_song;

#endif

