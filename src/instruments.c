#include "instruments.h"

static const Instrument s_instruments[] = {
    {
        .kind = INSTRUMENT_SQUARE,
        .square = {
            .duty = 2,
            .volume = 6,
            .envelope_time = 4,
            .envelope_decrease = true,
        },
    },
    {
        .kind = INSTRUMENT_SQUARE,
        .square = {
            .duty = 1,
            .volume = 5,
            .envelope_time = 4,
            .envelope_decrease = true,
        },
    },
    {
        .kind = INSTRUMENT_WAVE,
        .wave = {
            .volume_code = 3,
            .wave_ram = { 0x6789A987, 0x65443210, 0x12345567, 0x89ABBA98 },
        },
    },
    {
        .kind = INSTRUMENT_NOISE,
        .noise = {
            .volume = 4,
            .envelope_time = 5,
            .envelope_decrease = true,
            .ratio = 3,
            .narrow = false,
            .shift = 10,
        },
    },
};

const Instrument *instrument_get(u8 index)
{
    if(index == 0 || index > countof(s_instruments))
        return NULL;

    return &s_instruments[index - 1];
}
