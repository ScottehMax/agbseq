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

static void editor_raise_octave(Editor *editor)
{
    if(editor->current_note <= 71)
        editor->current_note += 12;
}

static void editor_lower_note(Editor *editor)
{
    if(editor->current_note > 24)
        editor->current_note--;
}

static void editor_lower_octave(Editor *editor)
{
    if(editor->current_note >= 36)
        editor->current_note -= 12;
}

static PatternCell *editor_cursor_cell(Editor *editor, Song *song)
{
    const u8 pattern_index = song->order[0];

    return &song->patterns[pattern_index].cells[editor->cursor_row][editor->cursor_track];
}

static void editor_toggle_mode(Editor *editor)
{
    editor->mode = editor->mode == EDITOR_MODE_NOTE ? EDITOR_MODE_EFFECT : EDITOR_MODE_NOTE;
}

static void editor_cycle_effect(Editor *editor, Song *song, Sequencer *sequencer)
{
    PatternCell *cell = editor_cursor_cell(editor, song);

    switch(cell->effect)
    {
    case EFFECT_NONE:
        cell->effect = EFFECT_SET_SPEED;
        cell->param = sequencer->frames_per_row;
        break;
    case EFFECT_SET_SPEED:
        cell->effect = EFFECT_NOTE_CUT;
        cell->param = 0;
        break;
    default:
        cell->effect = EFFECT_NONE;
        cell->param = 0;
        break;
    }

    editor->song_dirty = true;
}

static void editor_clear_effect(Editor *editor, Song *song)
{
    PatternCell *cell = editor_cursor_cell(editor, song);

    cell->effect = EFFECT_NONE;
    cell->param = 0;
    editor->song_dirty = true;
}

static void editor_adjust_effect_param(Editor *editor, Song *song, s8 delta)
{
    PatternCell *cell = editor_cursor_cell(editor, song);

    if(cell->effect != EFFECT_SET_SPEED)
        return;

    if(delta < 0 && cell->param > 2)
        cell->param--;
    else if(delta > 0 && cell->param < 30)
        cell->param++;
    else
        return;

    editor->song_dirty = true;
}

static void editor_pickup_cell_note(Editor *editor, Song *song)
{
    const PatternCell *cell = editor_cursor_cell(editor, song);

    if(cell->note != SONG_EMPTY_NOTE)
        editor->current_note = cell->note;
}

static void editor_apply_pitch_to_cell(Editor *editor, Song *song)
{
    PatternCell *cell = editor_cursor_cell(editor, song);

    if(cell->note == SONG_EMPTY_NOTE)
        return;

    cell->note = editor->current_note;
    cell->instrument = editor_default_instrument(editor->cursor_track);
    audio_trigger_note((AudioChannel)editor->cursor_track, cell->note, instrument_get(cell->instrument));
    editor->song_dirty = true;
}

static void editor_advance_row(Editor *editor)
{
    editor->cursor_row++;
    if(editor->cursor_row >= SONG_PATTERN_ROWS)
        editor->cursor_row = 0;
}

void editor_init(Editor *editor)
{
    editor->cursor_row = 0;
    editor->cursor_track = 0;
    editor->current_note = 48;
    editor->mode = EDITOR_MODE_NOTE;
    editor->song_dirty = true;
}

void editor_update(Editor *editor, Song *song, Sequencer *sequencer, const InputState *input)
{
    const bool select_held = (input->held & KEY_SELECT) != 0;
    const u8 previous_track = editor->cursor_track;

    editor->song_dirty = false;

    if(select_held)
    {
        if((input->hit & KEY_SELECT) != 0 && (input->hit & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT)) == 0)
            editor_toggle_mode(editor);
        if(input->hit & KEY_UP)
            sequencer_set_tempo(sequencer, sequencer->frames_per_row > 2 ? sequencer->frames_per_row - 1 : 2);
        if(input->hit & KEY_DOWN)
            sequencer_set_tempo(sequencer, sequencer->frames_per_row < 30 ? sequencer->frames_per_row + 1 : 30);
        if(input->hit & KEY_LEFT)
        {
            editor_lower_octave(editor);
            editor_apply_pitch_to_cell(editor, song);
        }
        if(input->hit & KEY_RIGHT)
        {
            editor_raise_octave(editor);
            editor_apply_pitch_to_cell(editor, song);
        }
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
        editor_advance_row(editor);

    if((input->hit & (KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN)) != 0)
        editor_pickup_cell_note(editor, song);

    if(input->hit & KEY_L)
    {
        if(editor->mode == EDITOR_MODE_EFFECT)
        {
            editor_adjust_effect_param(editor, song, -1);
            return;
        }

        editor_lower_note(editor);
        editor_apply_pitch_to_cell(editor, song);
        if(!editor->song_dirty)
            audio_trigger_note((AudioChannel)editor->cursor_track,
                editor->current_note,
                instrument_get(editor_default_instrument(editor->cursor_track)));
    }

    if(input->hit & KEY_R)
    {
        if(editor->mode == EDITOR_MODE_EFFECT)
        {
            editor_adjust_effect_param(editor, song, 1);
            return;
        }

        editor_raise_note(editor);
        editor_apply_pitch_to_cell(editor, song);
        if(!editor->song_dirty)
            audio_trigger_note((AudioChannel)editor->cursor_track,
                editor->current_note,
                instrument_get(editor_default_instrument(editor->cursor_track)));
    }

    if(input->hit & KEY_A)
    {
        if(editor->mode == EDITOR_MODE_EFFECT)
        {
            editor_cycle_effect(editor, song, sequencer);
            return;
        }

        PatternCell *cell = editor_cursor_cell(editor, song);
        cell->note = editor->current_note;
        cell->instrument = editor_default_instrument(editor->cursor_track);
        cell->effect = EFFECT_NONE;
        cell->param = 0;
        audio_trigger_note((AudioChannel)editor->cursor_track, cell->note, instrument_get(cell->instrument));
        editor->song_dirty = true;
        editor_advance_row(editor);
    }

    if(input->hit & KEY_B)
    {
        if(editor->mode == EDITOR_MODE_EFFECT)
        {
            editor_clear_effect(editor, song);
            return;
        }

        PatternCell *cell = editor_cursor_cell(editor, song);
        cell->note = SONG_EMPTY_NOTE;
        cell->instrument = 0;
        cell->effect = EFFECT_NONE;
        cell->param = 0;
        audio_stop_channel((AudioChannel)editor->cursor_track);
        editor->song_dirty = true;
    }
}
