#include "ui.h"

#include <stdio.h>

#define UI_VISIBLE_ROWS 12
#define UI_ROW_TOP 40
#define UI_ROW_HEIGHT 9
#define UI_TRACK_WIDTH 44
#define UI_TRACK_GAP 6

#define UI_BG RGB15(3, 5, 7)
#define UI_PANEL RGB15(7, 10, 13)
#define UI_PANEL_DARK RGB15(4, 7, 10)
#define UI_CELL RGB15(9, 12, 15)
#define UI_CELL_ALT RGB15(8, 10, 13)
#define UI_SELECTED RGB15(24, 20, 7)
#define UI_PLAY_ROW RGB15(5, 15, 13)
#define UI_TEXT RGB15(27, 29, 29)
#define UI_TEXT_DIM RGB15(16, 19, 20)
#define UI_ACCENT RGB15(30, 18, 7)
#define UI_CLEAR RGB15(11, 8, 9)

typedef struct UiState {
    bool initialized;
    bool playing;
    u8 frames_per_row;
    u8 pattern_index;
    u8 row;
    u8 cursor_row;
    u8 cursor_track;
    u8 current_note;
    u8 visible_start;
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

static u8 ui_visible_start(u8 cursor_row)
{
    if(cursor_row >= SONG_PATTERN_ROWS - UI_VISIBLE_ROWS)
        return SONG_PATTERN_ROWS - UI_VISIBLE_ROWS;

    return 0;
}

static bool ui_row_visible(u8 row, u8 visible_start)
{
    return row >= visible_start && row < visible_start + UI_VISIBLE_ROWS;
}

static int ui_row_y(u8 row, u8 visible_start)
{
    return UI_ROW_TOP + (row - visible_start) * UI_ROW_HEIGHT;
}

static int ui_track_x(u8 track)
{
    return 34 + track * (UI_TRACK_WIDTH + UI_TRACK_GAP);
}

static void ui_text(int x, int y, COLOR color)
{
    tte_set_ink(color);
    tte_set_pos(x, y);
}

static void ui_draw_shell(void)
{
    m3_fill(UI_BG);
    m3_rect(0, 0, 240, 28, UI_PANEL);
    m3_rect(0, 28, 240, 39, UI_PANEL_DARK);
    m3_rect(0, 148, 240, 160, UI_PANEL);

    ui_text(4, 30, UI_TEXT_DIM);
    tte_printf("ROW");

    static const char *tracks[] = { "CH1", "CH2", "WAV", "NOI" };
    for(u8 track = 0; track < SONG_TRACK_COUNT; track++)
    {
        ui_text(ui_track_x(track) + 12, 30, UI_TEXT_DIM);
        tte_printf("%s", tracks[track]);
    }

    ui_text(4, 149, UI_TEXT_DIM);
    tte_printf("A add  B clear  L/R note  SEL+D");
}

static void ui_draw_status(const Sequencer *sequencer, const Editor *editor, u8 pattern_index)
{
    m3_rect(0, 0, 240, 28, UI_PANEL);
    m3_rect(4, 4, 52, 22, sequencer->playing ? UI_PLAY_ROW : UI_CLEAR);
    ui_text(11, 8, UI_TEXT);
    tte_printf("%s", sequencer->playing ? "PLAY" : "STOP");

    ui_text(62, 4, UI_TEXT_DIM);
    tte_printf("speed");
    ui_text(62, 14, UI_TEXT);
    tte_printf("%02u", sequencer->frames_per_row);

    ui_text(102, 4, UI_TEXT_DIM);
    tte_printf("row");
    ui_text(102, 14, UI_TEXT);
    tte_printf("%02u", sequencer->row);

    ui_text(138, 4, UI_TEXT_DIM);
    tte_printf("cursor");
    ui_text(138, 14, UI_TEXT);
    tte_printf("R%02u C%u", editor->cursor_row, editor->cursor_track + 1);

    ui_text(198, 4, UI_TEXT_DIM);
    tte_printf("note");
    ui_text(198, 14, UI_ACCENT);
    tte_printf("%s%u", ui_note_name(editor->current_note), ui_note_octave(editor->current_note));

    (void)pattern_index;
}

static void ui_draw_cell(const PatternCell *cell, u8 track, int y, bool selected)
{
    const int x = ui_track_x(track);
    const COLOR fill = selected ? UI_SELECTED : (track & 1 ? UI_CELL_ALT : UI_CELL);
    const COLOR text = selected ? UI_BG : UI_TEXT;

    m3_rect(x, y, x + UI_TRACK_WIDTH, y + UI_ROW_HEIGHT, fill);

    ui_text(x + 10, y - 1, text);
    if(cell->note == SONG_EMPTY_NOTE)
        tte_printf("...");
    else
        tte_printf("%s%u", ui_note_name(cell->note), ui_note_octave(cell->note));
}

static void ui_draw_pattern_row(
    const Sequencer *sequencer,
    const Editor *editor,
    const Pattern *pattern,
    u8 visible_start,
    u8 row)
{
    if(!ui_row_visible(row, visible_start))
        return;

    const int y = ui_row_y(row, visible_start);
    const bool play_row = row == sequencer->row;

    m3_rect(0, y, 240, y + UI_ROW_HEIGHT, play_row ? UI_PLAY_ROW : UI_BG);
    m3_rect(0, y, 30, y + UI_ROW_HEIGHT, play_row ? UI_PLAY_ROW : UI_PANEL_DARK);

    ui_text(6, y - 1, play_row ? UI_TEXT : UI_TEXT_DIM);
    tte_printf("%02u", row);

    for(u8 track = 0; track < SONG_TRACK_COUNT; track++)
    {
        const bool selected = row == editor->cursor_row && track == editor->cursor_track;
        ui_draw_cell(&pattern->cells[row][track], track, y, selected);
    }
}

static void ui_draw_grid(const Sequencer *sequencer, const Editor *editor, u8 pattern_index, u8 visible_start)
{
    const Pattern *pattern = &sequencer->song->patterns[pattern_index];

    m3_rect(0, UI_ROW_TOP, 240, 150, UI_BG);

    for(u8 offset = 0; offset < UI_VISIBLE_ROWS; offset++)
        ui_draw_pattern_row(sequencer, editor, pattern, visible_start, visible_start + offset);
}

void ui_init(void)
{
    REG_DISPCNT = DCNT_MODE3 | DCNT_BG2;
    tte_init_bmp_default(3);
    tte_init_con();
    s_ui_state.initialized = false;
}

void ui_invalidate(void)
{
    s_ui_state.initialized = false;
}

void ui_render(const Sequencer *sequencer, const Editor *editor)
{
    const u8 pattern_index = sequencer->song->order[sequencer->order_index];
    const u8 visible_start = ui_visible_start(editor->cursor_row);

    if(!s_ui_state.initialized
        || s_ui_state.pattern_index != pattern_index
        || s_ui_state.visible_start != visible_start)
    {
        ui_draw_shell();
        ui_draw_status(sequencer, editor, pattern_index);
        ui_draw_grid(sequencer, editor, pattern_index, visible_start);
        s_ui_state.initialized = true;
        s_ui_state.playing = sequencer->playing;
        s_ui_state.frames_per_row = sequencer->frames_per_row;
        s_ui_state.pattern_index = pattern_index;
        s_ui_state.row = sequencer->row;
        s_ui_state.cursor_row = editor->cursor_row;
        s_ui_state.cursor_track = editor->cursor_track;
        s_ui_state.current_note = editor->current_note;
        s_ui_state.visible_start = visible_start;
        return;
    }

    if(s_ui_state.playing != sequencer->playing
        || s_ui_state.frames_per_row != sequencer->frames_per_row
        || s_ui_state.row != sequencer->row
        || s_ui_state.cursor_row != editor->cursor_row
        || s_ui_state.cursor_track != editor->cursor_track
        || s_ui_state.current_note != editor->current_note)
    {
        ui_draw_status(sequencer, editor, pattern_index);
    }

    const Pattern *pattern = &sequencer->song->patterns[pattern_index];

    if(s_ui_state.row != sequencer->row)
    {
        ui_draw_pattern_row(sequencer, editor, pattern, visible_start, s_ui_state.row);
        ui_draw_pattern_row(sequencer, editor, pattern, visible_start, sequencer->row);
    }

    if(editor->song_dirty)
        ui_draw_pattern_row(sequencer, editor, pattern, visible_start, editor->cursor_row);

    if(s_ui_state.cursor_row != editor->cursor_row || s_ui_state.cursor_track != editor->cursor_track)
    {
        if(s_ui_state.cursor_row == editor->cursor_row)
        {
            ui_draw_pattern_row(sequencer, editor, pattern, visible_start, editor->cursor_row);
        }
        else
        {
            ui_draw_pattern_row(sequencer, editor, pattern, visible_start, s_ui_state.cursor_row);
            ui_draw_pattern_row(sequencer, editor, pattern, visible_start, editor->cursor_row);
        }
    }

    s_ui_state.playing = sequencer->playing;
    s_ui_state.frames_per_row = sequencer->frames_per_row;
    s_ui_state.pattern_index = pattern_index;
    s_ui_state.row = sequencer->row;
    s_ui_state.cursor_row = editor->cursor_row;
    s_ui_state.cursor_track = editor->cursor_track;
    s_ui_state.current_note = editor->current_note;
    s_ui_state.visible_start = visible_start;
}
