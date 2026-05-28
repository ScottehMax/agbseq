#include <tonc.h>

#include "audio.h"
#include "editor.h"
#include "input.h"
#include "sequencer.h"
#include "song.h"
#include "ui.h"

int main(void)
{
    Sequencer sequencer;
    Editor editor;
    Song song = g_demo_song;
    InputState input = { 0 };

    irq_init(NULL);
    irq_add(II_VBLANK, NULL);

    audio_init();
    ui_init();
    editor_init(&editor);
    sequencer_init(&sequencer, &song);

    while(1)
    {
        VBlankIntrWait();

        input_update(&input);

        if(input.hit & KEY_START)
            sequencer_toggle_play(&sequencer);

        editor_update(&editor, &song, &sequencer, &input);

        sequencer_update(&sequencer);
        ui_render(&sequencer, &editor);
    }
}
