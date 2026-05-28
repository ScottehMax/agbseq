#include "audio.h"

#include "song.h"

static u16 audio_square_control(const SquareInstrument *instrument)
{
    const u16 direction = instrument->envelope_decrease ? SSQR_DEC : SSQR_INC;

    return SSQR_IVOL(instrument->volume & 15)
        | direction
        | SSQR_TIME(instrument->envelope_time & 7)
        | SSQR_DUTY(instrument->duty & 3);
}

static u16 audio_note_rate(u8 note)
{
    const u8 semitone = note % 12;
    const int octave = (note / 12) - 2;

    return SND_RATE(semitone, octave) & SFREQ_RATE_MASK;
}

static void audio_write_wave_ram(const WaveInstrument *instrument)
{
    REG_WAVE_RAM0 = instrument->wave_ram[0];
    REG_WAVE_RAM1 = instrument->wave_ram[1];
    REG_WAVE_RAM2 = instrument->wave_ram[2];
    REG_WAVE_RAM3 = instrument->wave_ram[3];
}

void audio_init(void)
{
    REG_SNDSTAT = SSTAT_ENABLE;
    REG_SNDDMGCNT = SDMG_BUILD_LR(SDMG_SQR1 | SDMG_SQR2 | SDMG_WAVE | SDMG_NOISE, 4);
    REG_SNDDSCNT = SDS_DMG50;
    audio_silence();
}

void audio_silence(void)
{
    REG_SND1CNT = 0;
    REG_SND1FREQ = 0;
    REG_SND2CNT = 0;
    REG_SND2FREQ = 0;
    REG_SND3SEL = 0;
    REG_SND3CNT = 0;
    REG_SND3FREQ = 0;
    REG_SND4CNT = 0;
    REG_SND4FREQ = 0;
}

void audio_trigger_note(AudioChannel channel, u8 note, const Instrument *instrument)
{
    if(note == SONG_EMPTY_NOTE || instrument == NULL)
        return;

    switch(channel)
    {
    case AUDIO_CH_SQUARE1:
        if(instrument->kind != INSTRUMENT_SQUARE)
            return;
        REG_SND1SWEEP = SSW_OFF;
        REG_SND1CNT = audio_square_control(&instrument->square);
        REG_SND1FREQ = audio_note_rate(note) | SFREQ_RESET;
        break;

    case AUDIO_CH_SQUARE2:
        if(instrument->kind != INSTRUMENT_SQUARE)
            return;
        REG_SND2CNT = audio_square_control(&instrument->square);
        REG_SND2FREQ = audio_note_rate(note) | SFREQ_RESET;
        break;

    case AUDIO_CH_WAVE:
        if(instrument->kind != INSTRUMENT_WAVE)
            return;
        REG_SND3SEL = BIT(6);
        audio_write_wave_ram(&instrument->wave);
        REG_SND3CNT = (instrument->wave.volume_code & 3) << 13;
        REG_SND3FREQ = audio_note_rate(note) | SFREQ_RESET;
        REG_SND3SEL = BIT(7);
        break;

    case AUDIO_CH_NOISE:
        if(instrument->kind != INSTRUMENT_NOISE)
            return;
        REG_SND4CNT = SSQR_IVOL(instrument->noise.volume & 15)
            | (instrument->noise.envelope_decrease ? SSQR_DEC : SSQR_INC)
            | SSQR_TIME(instrument->noise.envelope_time & 7);
        REG_SND4FREQ = (instrument->noise.ratio & 7)
            | (instrument->noise.narrow ? BIT(3) : 0)
            | ((instrument->noise.shift & 15) << 4)
            | SFREQ_RESET;
        break;

    default:
        break;
    }
}

void audio_stop_channel(AudioChannel channel)
{
    switch(channel)
    {
    case AUDIO_CH_SQUARE1:
        REG_SND1CNT = 0;
        REG_SND1FREQ = 0;
        break;
    case AUDIO_CH_SQUARE2:
        REG_SND2CNT = 0;
        REG_SND2FREQ = 0;
        break;
    case AUDIO_CH_WAVE:
        REG_SND3CNT = 0;
        REG_SND3FREQ = 0;
        break;
    case AUDIO_CH_NOISE:
        REG_SND4CNT = 0;
        REG_SND4FREQ = 0;
        break;
    default:
        break;
    }
}
