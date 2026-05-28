#include "ui.h"

#include <stdio.h>

#define UI_VISIBLE_ROWS 12
#define UI_ROW_TOP 40
#define UI_ROW_HEIGHT 9
#define UI_TRACK_WIDTH 44
#define UI_TRACK_GAP 6
#define UI_TEXT_CBB 0
#define UI_COLOR_CBB 2
#define UI_TEXT_SBB 31
#define UI_COLOR_SBB 30
#define UI_TEXT_PAL_BANK 15
#define UI_SURFACE_WORDS ((240 * 160) / 8)
#define UI_TILE_COLS 30
#define UI_TILE_ROWS 20
#define UI_TILE_WORDS 8

enum UiTextColor {
    UI_TEXT_PAL_CLEAR = 0,
    UI_TEXT_PAL_MAIN = 1,
    UI_TEXT_PAL_DIM = 2,
    UI_TEXT_PAL_ACCENT = 3,
};

enum UiColor {
    UI_BG = 1,
    UI_PANEL,
    UI_PANEL_DARK,
    UI_CELL,
    UI_CELL_ALT,
    UI_SELECTED,
    UI_PLAY_ROW,
    UI_TEXT,
    UI_TEXT_DIM,
    UI_ACCENT,
    UI_CLEAR,
};

typedef struct UiState {
    bool initialized;
    bool playing;
    u8 frames_per_row;
    u8 pattern_index;
    u8 row;
    u8 cursor_row;
    u8 cursor_track;
    u8 current_note;
    u8 mode;
    bool help_visible;
    u8 visible_start;
} UiState;

typedef struct UiDirtyRect {
    bool dirty;
    u8 left;
    u8 top;
    u8 right;
    u8 bottom;
} UiDirtyRect;

static UiState s_ui_state = {
    .initialized = false,
};

static TSurface s_color_surface;
static EWRAM_BSS u32 s_color_tiles[UI_SURFACE_WORDS];
static EWRAM_BSS u32 s_text_tiles[UI_SURFACE_WORDS];
static UiDirtyRect s_color_dirty;
static UiDirtyRect s_text_dirty;

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

static const char *ui_mode_name(const Editor *editor)
{
    return editor->mode == EDITOR_MODE_EFFECT ? "FX" : "NOTE";
}

static void ui_effect_text(const PatternCell *cell, char *dst)
{
    switch(cell->effect)
    {
    case EFFECT_SET_SPEED:
        siprintf(dst, "S%02u", cell->param);
        break;
    case EFFECT_NOTE_CUT:
        siprintf(dst, "CUT");
        break;
    default:
        siprintf(dst, "---");
        break;
    }
}

static u8 ui_visible_start(u8 cursor_row)
{
    if(cursor_row >= UI_VISIBLE_ROWS)
        return cursor_row - UI_VISIBLE_ROWS + 1;

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

static void ui_mark_dirty(UiDirtyRect *dirty, int left, int top, int right, int bottom)
{
    if(left < 0)
        left = 0;
    if(top < 0)
        top = 0;
    if(right > 240)
        right = 240;
    if(bottom > 160)
        bottom = 160;
    if(left >= right || top >= bottom)
        return;

    const u8 tile_left = left >> 3;
    const u8 tile_top = top >> 3;
    const u8 tile_right = (right + 7) >> 3;
    const u8 tile_bottom = (bottom + 7) >> 3;

    if(!dirty->dirty)
    {
        dirty->dirty = true;
        dirty->left = tile_left;
        dirty->top = tile_top;
        dirty->right = tile_right;
        dirty->bottom = tile_bottom;
        return;
    }

    if(tile_left < dirty->left)
        dirty->left = tile_left;
    if(tile_top < dirty->top)
        dirty->top = tile_top;
    if(tile_right > dirty->right)
        dirty->right = tile_right;
    if(tile_bottom > dirty->bottom)
        dirty->bottom = tile_bottom;
}

static void ui_mark_text_dirty(int left, int top, int right, int bottom)
{
    ui_mark_dirty(&s_text_dirty, left, top, right, bottom);
}

static void ui_text(int x, int y, u8 color)
{
    u8 text_color = UI_TEXT_PAL_MAIN;

    if(color == UI_TEXT_DIM)
        text_color = UI_TEXT_PAL_DIM;
    else if(color == UI_ACCENT)
        text_color = UI_TEXT_PAL_ACCENT;

    tte_set_ink(text_color);
    tte_set_pos(x, y);
    ui_mark_text_dirty(x, y, x + 140, y + 12);
}

static void ui_bind_text_target(void)
{
    tte_get_context()->dst.data = (u8*)s_text_tiles;
}

static void ui_rect(int left, int top, int right, int bottom, u8 color)
{
    schr4c_rect(&s_color_surface, left, top, right, bottom, color);
    ui_mark_dirty(&s_color_dirty, left, top, right, bottom);
}

static void ui_clear_text(void)
{
    memset32(s_text_tiles, 0, UI_SURFACE_WORDS);
    ui_mark_text_dirty(0, 0, 240, 160);
}

static void ui_erase_text(int left, int top, int right, int bottom)
{
    chr4c_erase(left, top, right, bottom);
    ui_mark_text_dirty(left, top, right, bottom);
}

static void ui_draw_shell(void)
{
    ui_rect(0, 0, 240, 160, UI_BG);
    ui_rect(0, 0, 240, 28, UI_PANEL);
    ui_rect(0, 28, 240, 39, UI_PANEL_DARK);
    ui_rect(0, 148, 240, 160, UI_PANEL);

    ui_text(4, 28, UI_TEXT_DIM);
    tte_printf("ROW");

    static const char *tracks[] = { "CH1", "CH2", "WAV", "NOI" };
    for(u8 track = 0; track < SONG_TRACK_COUNT; track++)
    {
        ui_text(ui_track_x(track) + 12, 28, UI_TEXT_DIM);
        tte_printf("%s", tracks[track]);
    }

    ui_text(4, 149, UI_TEXT_DIM);
    tte_printf("SEL mode  A set  B clear  L/R edit");
}

static void ui_draw_status(const Sequencer *sequencer, const Editor *editor, u8 pattern_index)
{
    ui_rect(0, 0, 240, 28, UI_PANEL);
    ui_rect(4, 4, 52, 22, sequencer->playing ? UI_PLAY_ROW : UI_CLEAR);
    ui_erase_text(0, 0, 240, 28);
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
    tte_printf("%s", ui_mode_name(editor));
    ui_text(138, 14, UI_TEXT);
    tte_printf("R%02u C%u", editor->cursor_row, editor->cursor_track + 1);

    ui_text(198, 4, UI_TEXT_DIM);
    tte_printf("note");
    ui_text(198, 14, UI_ACCENT);
    tte_printf("%s%u", ui_note_name(editor->current_note), ui_note_octave(editor->current_note));

    (void)pattern_index;
}

static void ui_draw_status_row(const Sequencer *sequencer)
{
    ui_erase_text(100, 14, 128, 26);
    ui_text(102, 14, UI_TEXT);
    tte_printf("%02u", sequencer->row);
}

static void ui_draw_cell_backdrop(u8 track, int y, bool selected)
{
    const int x = ui_track_x(track);
    const u8 fill = selected ? UI_SELECTED : (track & 1 ? UI_CELL_ALT : UI_CELL);

    ui_rect(x, y, x + UI_TRACK_WIDTH, y + UI_ROW_HEIGHT, fill);
}

static void ui_draw_cell_text(const PatternCell *cell, u8 track, int y, bool show_effect)
{
    const int x = ui_track_x(track);
    char effect_text[4];

    ui_text(x + 10, y - 2, UI_TEXT);
    tte_printf("    ");
    ui_text(x + 10, y - 2, UI_TEXT);
    if(show_effect)
    {
        ui_effect_text(cell, effect_text);
        tte_printf("%s", effect_text);
    }
    else if(cell->note == SONG_EMPTY_NOTE)
    {
        tte_printf("...");
    }
    else
    {
        tte_printf("%s%u", ui_note_name(cell->note), ui_note_octave(cell->note));
    }
}

static void ui_draw_pattern_row_backdrop(
    const Sequencer *sequencer,
    const Editor *editor,
    u8 visible_start,
    u8 row)
{
    if(!ui_row_visible(row, visible_start))
        return;

    const int y = ui_row_y(row, visible_start);
    const bool play_row = row == sequencer->row;

    ui_rect(0, y, 240, y + UI_ROW_HEIGHT, play_row ? UI_PLAY_ROW : UI_BG);
    ui_rect(0, y, 30, y + UI_ROW_HEIGHT, play_row ? UI_PLAY_ROW : UI_PANEL_DARK);

    for(u8 track = 0; track < SONG_TRACK_COUNT; track++)
    {
        const bool selected = row == editor->cursor_row && track == editor->cursor_track;
        ui_draw_cell_backdrop(track, y, selected);
    }
}

static void ui_draw_pattern_row_text(
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

    ui_text(6, y - 2, play_row ? UI_TEXT : UI_TEXT_DIM);
    tte_printf("%02u", row);

    for(u8 track = 0; track < SONG_TRACK_COUNT; track++)
    {
        ui_draw_cell_text(
            &pattern->cells[row][track],
            track,
            y,
            editor->mode == EDITOR_MODE_EFFECT);
    }
}

static void ui_draw_pattern_row(
    const Sequencer *sequencer,
    const Editor *editor,
    const Pattern *pattern,
    u8 visible_start,
    u8 row)
{
    ui_draw_pattern_row_backdrop(sequencer, editor, visible_start, row);
    ui_draw_pattern_row_text(sequencer, editor, pattern, visible_start, row);
}

static void ui_draw_grid(const Sequencer *sequencer, const Editor *editor, u8 pattern_index, u8 visible_start)
{
    const Pattern *pattern = &sequencer->song->patterns[pattern_index];

    ui_rect(0, UI_ROW_TOP, 240, 150, UI_BG);
    ui_erase_text(0, UI_ROW_TOP - 2, 240, 150);

    for(u8 offset = 0; offset < UI_VISIBLE_ROWS; offset++)
        ui_draw_pattern_row(sequencer, editor, pattern, visible_start, visible_start + offset);
}

static void ui_draw_full_editor(const Sequencer *sequencer, const Editor *editor, u8 pattern_index, u8 visible_start)
{
    ui_clear_text();
    ui_draw_shell();
    ui_draw_status(sequencer, editor, pattern_index);
    ui_draw_grid(sequencer, editor, pattern_index, visible_start);
}

static void ui_capture_state(
    UiState *state,
    const Sequencer *sequencer,
    const Editor *editor,
    u8 pattern_index,
    u8 visible_start)
{
    state->initialized = true;
    state->playing = sequencer->playing;
    state->frames_per_row = sequencer->frames_per_row;
    state->pattern_index = pattern_index;
    state->row = sequencer->row;
    state->cursor_row = editor->cursor_row;
    state->cursor_track = editor->cursor_track;
    state->current_note = editor->current_note;
    state->mode = editor->mode;
    state->help_visible = editor->help_visible;
    state->visible_start = visible_start;
}

static void ui_draw_incremental_editor(
    const Sequencer *sequencer,
    const Editor *editor,
    u8 pattern_index,
    u8 visible_start,
    const UiState *previous)
{
    if(previous->playing != sequencer->playing
        || previous->frames_per_row != sequencer->frames_per_row
        || previous->cursor_row != editor->cursor_row
        || previous->cursor_track != editor->cursor_track
        || previous->current_note != editor->current_note)
    {
        ui_draw_status(sequencer, editor, pattern_index);
    }
    else if(previous->row != sequencer->row)
    {
        ui_draw_status_row(sequencer);
    }

    if(previous->row != sequencer->row)
    {
        const Pattern *pattern = &sequencer->song->patterns[pattern_index];

        ui_draw_pattern_row_backdrop(sequencer, editor, visible_start, previous->row);
        ui_draw_pattern_row_backdrop(sequencer, editor, visible_start, sequencer->row);
        ui_draw_pattern_row_text(sequencer, editor, pattern, visible_start, previous->row);
        ui_draw_pattern_row_text(sequencer, editor, pattern, visible_start, sequencer->row);
    }

    if(previous->cursor_row != editor->cursor_row || previous->cursor_track != editor->cursor_track)
    {
        ui_draw_pattern_row_backdrop(sequencer, editor, visible_start, previous->cursor_row);
        ui_draw_pattern_row_backdrop(sequencer, editor, visible_start, editor->cursor_row);
    }

    if(editor->song_dirty)
        ui_draw_grid(sequencer, editor, pattern_index, visible_start);
}

static void ui_help_line(int y, const char *left, const char *right)
{
    ui_text(18, y, UI_ACCENT);
    tte_printf("%s", left);
    ui_text(110, y, UI_TEXT);
    tte_printf("%s", right);
}

static void ui_draw_help(void)
{
    ui_clear_text();
    ui_rect(0, 0, 240, 160, UI_BG);
    ui_rect(0, 0, 240, 26, UI_PANEL);
    ui_rect(0, 148, 240, 160, UI_PANEL);
    ui_erase_text(0, 0, 240, 160);

    ui_text(12, 8, UI_TEXT);
    tte_printf("agbseq help");
    ui_text(96, 8, UI_TEXT_DIM);
    tte_printf("pattern sequencer");

    ui_help_line(34, "START", "play / pause");
    ui_help_line(46, "D-PAD", "move cursor");
    ui_help_line(58, "A", "set note or cycle FX");
    ui_help_line(70, "B", "clear note or FX");
    ui_help_line(82, "L / R", "edit pitch or FX value");
    ui_help_line(94, "SELECT", "switch NOTE / FX mode");
    ui_help_line(106, "SEL+UP/DN", "change speed");
    ui_help_line(118, "SEL+LEFT/RIGHT", "octave down / up");
    ui_help_line(130, "SEL+L", "show this screen");

    ui_text(60, 148, UI_TEXT_DIM);
    tte_printf("Press any button to return");
}

void ui_init(void)
{
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1;
    REG_BG0CNT = BG_PRIO(0) | BG_CBB(UI_TEXT_CBB) | BG_SBB(UI_TEXT_SBB) | BG_4BPP | BG_REG_32x32;
    REG_BG1CNT = BG_PRIO(1) | BG_CBB(UI_COLOR_CBB) | BG_SBB(UI_COLOR_SBB) | BG_4BPP | BG_REG_32x32;
    CBB_CLEAR(UI_TEXT_CBB);
    CBB_CLEAR(UI_TEXT_CBB + 1);
    CBB_CLEAR(UI_COLOR_CBB);
    CBB_CLEAR(UI_COLOR_CBB + 1);
    memset32(s_color_tiles, 0, UI_SURFACE_WORDS);
    memset32(s_text_tiles, 0, UI_SURFACE_WORDS);
    pal_bg_mem[UI_BG] = RGB15(3, 5, 7);
    pal_bg_mem[UI_PANEL] = RGB15(7, 10, 13);
    pal_bg_mem[UI_PANEL_DARK] = RGB15(4, 7, 10);
    pal_bg_mem[UI_CELL] = RGB15(9, 12, 15);
    pal_bg_mem[UI_CELL_ALT] = RGB15(8, 10, 13);
    pal_bg_mem[UI_SELECTED] = RGB15(24, 20, 7);
    pal_bg_mem[UI_PLAY_ROW] = RGB15(5, 15, 13);
    pal_bg_mem[UI_CLEAR] = RGB15(11, 8, 9);
    srf_init(&s_color_surface, SRF_CHR4C, s_color_tiles, 240, 160, 4, pal_bg_mem);
    schr4c_prep_map(&s_color_surface, se_mem[UI_COLOR_SBB], 0);
    tte_init_chr4c(0, BG_CBB(UI_TEXT_CBB) | BG_SBB(UI_TEXT_SBB), UI_TEXT_PAL_BANK << 12, 0x0001, 0,
        &vwf_default, NULL);
    ui_bind_text_target();
    pal_bg_bank[UI_TEXT_PAL_BANK][UI_TEXT_PAL_CLEAR] = 0;
    pal_bg_bank[UI_TEXT_PAL_BANK][UI_TEXT_PAL_MAIN] = RGB15(27, 29, 29);
    pal_bg_bank[UI_TEXT_PAL_BANK][UI_TEXT_PAL_DIM] = RGB15(16, 19, 20);
    pal_bg_bank[UI_TEXT_PAL_BANK][UI_TEXT_PAL_ACCENT] = RGB15(30, 18, 7);
    tte_init_con();
    tte_set_paper(0);
    tte_set_shadow(0);
    tte_set_special(0);
    s_ui_state.initialized = false;
    ui_mark_dirty(&s_color_dirty, 0, 0, 240, 160);
    ui_mark_text_dirty(0, 0, 240, 160);
}

void ui_invalidate(void)
{
    s_ui_state.initialized = false;
}

void ui_render(const Sequencer *sequencer, const Editor *editor)
{
    const u8 pattern_index = sequencer->song->order[sequencer->order_index];
    const u8 visible_start = ui_visible_start(editor->cursor_row);
    const bool full_redraw = !s_ui_state.initialized
        || s_ui_state.pattern_index != pattern_index
        || s_ui_state.visible_start != visible_start
        || s_ui_state.mode != editor->mode
        || s_ui_state.help_visible != editor->help_visible;
    const bool dirty = !s_ui_state.initialized
        || s_ui_state.pattern_index != pattern_index
        || s_ui_state.visible_start != visible_start
        || s_ui_state.playing != sequencer->playing
        || s_ui_state.frames_per_row != sequencer->frames_per_row
        || s_ui_state.row != sequencer->row
        || s_ui_state.cursor_row != editor->cursor_row
        || s_ui_state.cursor_track != editor->cursor_track
        || s_ui_state.current_note != editor->current_note
        || s_ui_state.mode != editor->mode
        || s_ui_state.help_visible != editor->help_visible
        || editor->song_dirty;

    if(!dirty)
        return;

    ui_bind_text_target();

    if(editor->help_visible && !full_redraw)
    {
        ui_capture_state(&s_ui_state, sequencer, editor, pattern_index, visible_start);
        return;
    }

    if(full_redraw)
    {
        if(editor->help_visible)
            ui_draw_help();
        else
            ui_draw_full_editor(sequencer, editor, pattern_index, visible_start);

        ui_capture_state(&s_ui_state, sequencer, editor, pattern_index, visible_start);
        return;
    }

    const UiState previous = s_ui_state;
    ui_draw_incremental_editor(sequencer, editor, pattern_index, visible_start, &previous);

    ui_capture_state(&s_ui_state, sequencer, editor, pattern_index, visible_start);
}

static void ui_commit_dirty_rect(void *dst_tiles, const void *src_tiles, UiDirtyRect *dirty)
{
    if(!dirty->dirty)
        return;

    u32 *dst = (u32*)dst_tiles;
    const u32 *src = (const u32*)src_tiles;

    for(u8 tx = dirty->left; tx < dirty->right; tx++)
    {
        const uint tile_index = tx * UI_TILE_ROWS + dirty->top;
        const uint tile_count = dirty->bottom - dirty->top;
        memcpy32(
            &dst[tile_index * UI_TILE_WORDS],
            &src[tile_index * UI_TILE_WORDS],
            tile_count * UI_TILE_WORDS);
    }

    dirty->dirty = false;
}

void ui_commit(void)
{
    ui_commit_dirty_rect(tile_mem[UI_COLOR_CBB], s_color_tiles, &s_color_dirty);
    ui_commit_dirty_rect(tile_mem[UI_TEXT_CBB], s_text_tiles, &s_text_dirty);
}
