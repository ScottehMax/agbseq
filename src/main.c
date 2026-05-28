#include <tonc.h>

#include "audio.h"
#include "input.h"
#include "sequencer.h"
#include "song.h"
#include "ui.h"

int main(void)
{
    Sequencer sequencer;
    InputState input = { 0 };

    irq_init(NULL);
    irq_add(II_VBLANK, NULL);

    audio_init();
    ui_init();
    sequencer_init(&sequencer, &g_demo_song);

    while(1)
    {
        VBlankIntrWait();

        input_update(&input);

        if(input.hit & KEY_START)
            sequencer_toggle_play(&sequencer);

        if(input.hit & KEY_SELECT)
            sequencer_stop(&sequencer);

        if(input.hit & KEY_UP)
            sequencer_set_tempo(&sequencer, sequencer.frames_per_row > 2 ? sequencer.frames_per_row - 1 : 2);

        if(input.hit & KEY_DOWN)
            sequencer_set_tempo(&sequencer, sequencer.frames_per_row < 30 ? sequencer.frames_per_row + 1 : 30);

        sequencer_update(&sequencer);
        ui_render(&sequencer);
    }
}

