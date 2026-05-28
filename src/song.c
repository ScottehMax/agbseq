#include "song.h"

#define N(c, i) { (c), (i), EFFECT_NONE, 0 }
#define CUT() { SONG_EMPTY_NOTE, 0, EFFECT_NOTE_CUT, 0 }
#define EMPTY() { SONG_EMPTY_NOTE, 0, EFFECT_NONE, 0 }

const Song g_demo_song = {
    .initial_frames_per_row = 8,
    .order_length = SONG_ORDER_COUNT,
    .order = { 0 },
    .patterns = {
        {
            .cells = {
                { N(48, 1), N(36, 2), N(60, 3), N(1, 4) },
                { EMPTY(), EMPTY(), EMPTY(), EMPTY() },
                { N(52, 1), EMPTY(), EMPTY(), EMPTY() },
                { EMPTY(), EMPTY(), EMPTY(), EMPTY() },
                { N(55, 1), N(43, 2), N(64, 3), N(1, 4) },
                { EMPTY(), EMPTY(), EMPTY(), EMPTY() },
                { N(52, 1), EMPTY(), EMPTY(), EMPTY() },
                { EMPTY(), EMPTY(), EMPTY(), EMPTY() },
                { N(48, 1), N(36, 2), N(67, 3), N(1, 4) },
                { EMPTY(), EMPTY(), EMPTY(), EMPTY() },
                { N(55, 1), EMPTY(), EMPTY(), EMPTY() },
                { EMPTY(), EMPTY(), EMPTY(), EMPTY() },
                { N(57, 1), N(45, 2), N(64, 3), N(1, 4) },
                { EMPTY(), EMPTY(), EMPTY(), EMPTY() },
                { N(55, 1), EMPTY(), EMPTY(), CUT() },
                { EMPTY(), EMPTY(), EMPTY(), EMPTY() },
            },
        },
    },
};

