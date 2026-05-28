#ifndef AGBSEQ_EDITOR_H
#define AGBSEQ_EDITOR_H

#include <tonc.h>

#include "input.h"
#include "sequencer.h"
#include "song.h"

typedef struct Editor {
    u8 cursor_row;
    u8 cursor_track;
    u8 current_note;
    bool song_dirty;
} Editor;

void editor_init(Editor *editor);
void editor_update(Editor *editor, Song *song, Sequencer *sequencer, const InputState *input);

#endif
