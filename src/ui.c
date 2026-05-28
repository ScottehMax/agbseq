#include "ui.h"

#include <stdio.h>

typedef struct UiState {
    bool initialized;
    bool playing;
    u8 frames_per_row;
    u8 pattern_index;
    u8 row;
} UiState;

static UiState s_ui_state = {
    .initialized = false,
};

static const char *ui_note_name(u8 note)
{
    static const char *names[] = {
        "C-", "C#", "D-", "D#", "E-", "F-",
        "F#", "G-", "G#", "A-", "A#", "B-"
    };

    if(note == SONG_EMPTY_NOTE)
        return "--";

    return names[note % 12];
}

static u8 ui_note_octave(u8 note)
{
    return note / 12;
}

static void ui_print_status(const Sequencer *sequencer, u8 pattern_index)
{
    tte_set_pos(0, 0);
    tte_printf("AGBSEQ  %s  SPD:%02u  PAT:%02u ROW:%02u    ",
        sequencer->playing ? "PLAY" : "STOP",
        sequencer->frames_per_row,
        pattern_index,
        sequencer->row);
}

static void ui_print_row_marker(u8 row, bool selected)
{
    tte_set_pos(0, 32 + row * 8);
    tte_printf("%c", selected ? '>' : ' ');
}

static void ui_print_pattern_grid(const Sequencer *sequencer, u8 pattern_index)
{
    const Pattern *pattern = &sequencer->song->patterns[pattern_index];

    tte_erase_screen();
    ui_print_status(sequencer, pattern_index);
    tte_set_pos(0, 8);
    tte_printf("START play  SELECT stop  UP/DN speed    ");
    tte_set_pos(0, 24);
    tte_printf("    CH1    CH2    WAV    NOI");

    for(u8 row = 0; row < SONG_PATTERN_ROWS; row++)
    {
        tte_set_pos(0, 32 + row * 8);
        tte_printf("%c%02u ",
            row == sequencer->row ? '>' : ' ',
            row);

        for(u8 track = 0; track < SONG_TRACK_COUNT; track++)
        {
            const PatternCell *cell = &pattern->cells[row][track];
            if(cell->note == SONG_EMPTY_NOTE)
                tte_printf(" ...  ");
            else
                tte_printf(" %s%u  ", ui_note_name(cell->note), ui_note_octave(cell->note));
        }
    }
}

void ui_init(void)
{
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0;
    tte_init_se_default(0, BG_CBB(0) | BG_SBB(31));
    tte_init_con();
    s_ui_state.initialized = false;
}

void ui_render(const Sequencer *sequencer)
{
    const u8 pattern_index = sequencer->song->order[sequencer->order_index];

    if(!s_ui_state.initialized || s_ui_state.pattern_index != pattern_index)
    {
        ui_print_pattern_grid(sequencer, pattern_index);
        s_ui_state.initialized = true;
        s_ui_state.playing = sequencer->playing;
        s_ui_state.frames_per_row = sequencer->frames_per_row;
        s_ui_state.pattern_index = pattern_index;
        s_ui_state.row = sequencer->row;
        return;
    }

    if(s_ui_state.playing != sequencer->playing
        || s_ui_state.frames_per_row != sequencer->frames_per_row
        || s_ui_state.row != sequencer->row)
    {
        ui_print_status(sequencer, pattern_index);
    }

    if(s_ui_state.row != sequencer->row)
    {
        ui_print_row_marker(s_ui_state.row, false);
        ui_print_row_marker(sequencer->row, true);
    }

    s_ui_state.playing = sequencer->playing;
    s_ui_state.frames_per_row = sequencer->frames_per_row;
    s_ui_state.pattern_index = pattern_index;
    s_ui_state.row = sequencer->row;
}
