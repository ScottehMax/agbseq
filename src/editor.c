#include "editor.h"

#include "audio.h"
#include "instruments.h"

static u8 editor_default_instrument(u8 track)
{
    return track + 1;
}

static void editor_raise_note(Editor *editor)
{
    if(editor->current_note < 83)
        editor->current_note++;
}

static void editor_lower_note(Editor *editor)
{
    if(editor->current_note > 24)
        editor->current_note--;
}

static PatternCell *editor_cursor_cell(Editor *editor, Song *song)
{
    const u8 pattern_index = song->order[0];

    return &song->patterns[pattern_index].cells[editor->cursor_row][editor->cursor_track];
}

void editor_init(Editor *editor)
{
    editor->cursor_row = 0;
    editor->cursor_track = 0;
    editor->current_note = 48;
    editor->song_dirty = true;
}

void editor_update(Editor *editor, Song *song, Sequencer *sequencer, const InputState *input)
{
    const bool select_held = (input->held & KEY_SELECT) != 0;
    const u8 previous_track = editor->cursor_track;

    editor->song_dirty = false;

    if(select_held)
    {
        if(input->hit & KEY_UP)
            sequencer_set_tempo(sequencer, sequencer->frames_per_row > 2 ? sequencer->frames_per_row - 1 : 2);
        if(input->hit & KEY_DOWN)
            sequencer_set_tempo(sequencer, sequencer->frames_per_row < 30 ? sequencer->frames_per_row + 1 : 30);
        return;
    }

    if(input->hit & KEY_LEFT)
        editor->cursor_track = editor->cursor_track > 0 ? editor->cursor_track - 1 : SONG_TRACK_COUNT - 1;

    if(input->hit & KEY_RIGHT)
    {
        editor->cursor_track++;
        if(editor->cursor_track >= SONG_TRACK_COUNT)
            editor->cursor_track = 0;
    }

    if(previous_track != editor->cursor_track)
        audio_stop_channel((AudioChannel)previous_track);

    if(input->hit & KEY_UP)
        editor->cursor_row = editor->cursor_row > 0 ? editor->cursor_row - 1 : SONG_PATTERN_ROWS - 1;

    if(input->hit & KEY_DOWN)
    {
        editor->cursor_row++;
        if(editor->cursor_row >= SONG_PATTERN_ROWS)
            editor->cursor_row = 0;
    }

    if(input->hit & KEY_L)
    {
        editor_lower_note(editor);
        audio_trigger_note((AudioChannel)editor->cursor_track,
            editor->current_note,
            instrument_get(editor_default_instrument(editor->cursor_track)));
    }

    if(input->hit & KEY_R)
    {
        editor_raise_note(editor);
        audio_trigger_note((AudioChannel)editor->cursor_track,
            editor->current_note,
            instrument_get(editor_default_instrument(editor->cursor_track)));
    }

    if(input->hit & KEY_A)
    {
        PatternCell *cell = editor_cursor_cell(editor, song);
        cell->note = editor->current_note;
        cell->instrument = editor_default_instrument(editor->cursor_track);
        cell->effect = EFFECT_NONE;
        cell->param = 0;
        audio_trigger_note((AudioChannel)editor->cursor_track, cell->note, instrument_get(cell->instrument));
        editor->song_dirty = true;
    }

    if(input->hit & KEY_B)
    {
        PatternCell *cell = editor_cursor_cell(editor, song);
        cell->note = SONG_EMPTY_NOTE;
        cell->instrument = 0;
        cell->effect = EFFECT_NONE;
        cell->param = 0;
        audio_stop_channel((AudioChannel)editor->cursor_track);
        editor->song_dirty = true;
    }
}
