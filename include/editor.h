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
    u8 mode;
    u8 preview_frames;
    u8 preview_track;
    bool help_visible;
    bool song_dirty;
} Editor;

typedef enum EditorMode {
    EDITOR_MODE_NOTE = 0,
    EDITOR_MODE_EFFECT = 1
} EditorMode;

void editor_init(Editor *editor);
void editor_update(Editor *editor, Song *song, Sequencer *sequencer, const InputState *input);

#endif
