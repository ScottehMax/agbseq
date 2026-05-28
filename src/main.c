#include <tonc.h>

#include "audio.h"
#include "editor.h"
#include "input.h"
#include "sequencer.h"
#include "song.h"
#include "ui.h"

static Sequencer *s_playback_sequencer;

static void main_vblank_isr(void)
{
    if(s_playback_sequencer != NULL)
        sequencer_update(s_playback_sequencer);
}

int main(void)
{
    Sequencer sequencer;
    Editor editor;
    Song song = g_demo_song;
    InputState input = { 0 };

    audio_init();
    ui_init();
    editor_init(&editor);
    sequencer_init(&sequencer, &song);
    s_playback_sequencer = &sequencer;

    irq_init(NULL);
    irq_add(II_VBLANK, main_vblank_isr);

    while(1)
    {
        input_update(&input);

        const u16 ime = REG_IME;
        REG_IME = 0;

        const bool help_was_visible = editor.help_visible;

        if(!help_was_visible && (input.hit & KEY_START))
            sequencer_toggle_play(&sequencer);

        editor_update(&editor, &song, &sequencer, &input);

        const Sequencer render_sequencer = sequencer;

        REG_IME = ime;

        ui_render(&render_sequencer, &editor);

        VBlankIntrWait();
        ui_commit();
    }
}
