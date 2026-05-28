#include "input.h"

void input_update(InputState *input)
{
    key_poll();
    input->hit = (u16)key_hit(KEY_FULL);
    input->held = (u16)key_held(KEY_FULL);
}

