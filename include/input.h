#ifndef AGBSEQ_INPUT_H
#define AGBSEQ_INPUT_H

#include <tonc.h>

typedef struct InputState {
    u16 hit;
    u16 held;
} InputState;

void input_update(InputState *input);

#endif

