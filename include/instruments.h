#ifndef AGBSEQ_INSTRUMENTS_H
#define AGBSEQ_INSTRUMENTS_H

#include <tonc.h>

typedef enum InstrumentKind {
    INSTRUMENT_SQUARE = 0,
    INSTRUMENT_WAVE = 1,
    INSTRUMENT_NOISE = 2
} InstrumentKind;

typedef struct SquareInstrument {
    u8 duty;
    u8 volume;
    u8 envelope_time;
    bool envelope_decrease;
} SquareInstrument;

typedef struct WaveInstrument {
    u8 volume_code;
    u32 wave_ram[4];
} WaveInstrument;

typedef struct NoiseInstrument {
    u8 volume;
    u8 envelope_time;
    bool envelope_decrease;
    u8 ratio;
    bool narrow;
    u8 shift;
} NoiseInstrument;

typedef struct Instrument {
    InstrumentKind kind;
    union {
        SquareInstrument square;
        WaveInstrument wave;
        NoiseInstrument noise;
    };
} Instrument;

const Instrument *instrument_get(u8 index);

#endif

