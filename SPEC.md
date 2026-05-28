# GBA Music Sequencer Plan

## Goal

Build a native Game Boy Advance music sequencer that runs on hardware and emulators, using libtonc for GBA register definitions, memory maps, input, DMA helpers, IRQ helpers, video helpers, and basic runtime support.

This project should avoid ad-hoc redefinitions of GBA constants, register addresses, DMA flags, key bits, video modes, and sound control bits. Source files should include `<tonc.h>` and use libtonc symbols such as `REG_SNDSTAT`, `REG_SNDDMGCNT`, `REG_SNDDSCNT`, `REG_SND1CNT`, `REG_SND1FREQ`, `SSTAT_ENABLE`, `SDMG_*`, `SDS_*`, `SFREQ_RESET`, `KEY_*`, `REG_TM*`, `TM_*`, `irq_init`, `irq_add`, `VBlankIntrWait`, `vid_mem`, `tile_mem`, `se_mem`, `pal_bg_mem`, and `dma3_cpy` as appropriate.

## Assumptions

- The first version is a pattern-based sequencer, not a tracker clone with every advanced editing feature.
- The MVP uses the GBA hardware sound channels:
  - Channel 1: square wave with optional sweep.
  - Channel 2: square wave.
  - Channel 3: programmable wave channel.
  - Channel 4: noise/percussion.
- Direct Sound sample playback is a later phase because it requires timer-driven FIFO/DMA buffering and a small mixer or sample scheduler.
- The UI is built for the GBA screen and buttons, not for external editing on a PC.
- Persistence can initially be ROM-embedded demo songs. SRAM save/load can be added after the editor and playback engine are stable.
- libtonc is installed at `C:\devkitPro\libtonc`, with headers in `include` and `libtonc.a` in `lib`.

## Non-Goals for MVP

- MIDI import/export.
- Full sampled audio engine.
- Real-time audio effects beyond simple hardware-supported volume, envelope, duty, wave, and noise controls.
- Arbitrary project sizes that exceed EWRAM/ROM comfortably.
- Link cable sync.

## Target User Experience

The cartridge boots straight into the sequencer. The user can:

- Play and stop the current song.
- Move around a pattern grid.
- Edit notes, instrument numbers, and simple effect values.
- Switch between tracks/channels.
- Switch between patterns.
- Adjust tempo.
- Audition notes while editing.
- Hear deterministic playback on real hardware and emulators.

The first usable screen should be the editor, not a menu-heavy shell.

## Technical Architecture

### Build System

Create a devkitARM/libtonc C project:

- `Makefile`
  - Build an ARM7TDMI GBA ROM.
  - Include `C:\devkitPro\libtonc\include`.
  - Link against `C:\devkitPro\libtonc\lib\libtonc.a`.
  - Produce `agbseq.gba`.
- `src/main.c`
- `src/audio.c`, `src/audio.h`
- `src/song.c`, `src/song.h`
- `src/sequencer.c`, `src/sequencer.h`
- `src/ui.c`, `src/ui.h`
- `src/input.c`, `src/input.h`
- `src/instruments.c`, `src/instruments.h`

All code should compile as C, keeping data structures small and predictable.

### libtonc Usage Rules

- Include `<tonc.h>` for GBA types, registers, constants, input, IRQ, DMA, and video definitions.
- Do not define local versions of:
  - GBA integer typedefs already provided by libtonc.
  - Register addresses.
  - Sound bit masks.
  - DMA flags.
  - Timer flags.
  - IRQ masks.
  - Key masks.
  - VRAM/OAM/palette memory aliases.
- If a missing helper is needed, write a project-level helper that composes libtonc symbols rather than replacing them.

Example policy:

```c
#include <tonc.h>

REG_SNDSTAT = SSTAT_ENABLE;
REG_SNDDMGCNT = SDMG_BUILD_LR(SDMG_SQR1 | SDMG_SQR2 | SDMG_WAVE | SDMG_NOISE, 7);
REG_SND1FREQ = SND_RATE(note, octave) | SFREQ_RESET;
```

The exact macro availability should be checked against the installed libtonc headers during implementation.

## Sequencer Model

### Song

Represent a song as:

- Global tempo in BPM or ticks-per-row.
- Order list of pattern IDs.
- Pattern table.
- Instrument table.

Recommended initial limits:

- 4 tracks, mapped to the 4 hardware channels.
- 16 patterns.
- 64 rows per pattern.
- 16 instruments.
- 1 byte note value per cell.
- 1 byte instrument value per cell.
- 1 byte effect command per cell.
- 1 byte effect parameter per cell.

These limits keep memory usage modest and make the editor simple.

### Pattern Cell

Use a compact cell format:

```c
typedef struct PatternCell {
    u8 note;       // 0 = empty, otherwise semitone index.
    u8 instrument; // 0 = unchanged/no instrument, 1..15 valid.
    u8 effect;     // 0 = none.
    u8 param;      // effect-specific parameter.
} PatternCell;
```

### Instruments

Use channel-specific instrument definitions:

- Square instrument:
  - Duty.
  - Initial volume.
  - Envelope direction.
  - Envelope step time.
  - Optional sweep settings for channel 1.
- Wave instrument:
  - 32-sample 4-bit waveform.
  - Output level.
- Noise instrument:
  - Initial volume.
  - Envelope.
  - Noise mode/ratio/shift settings.

The instrument layer should translate abstract parameters to libtonc sound register writes.

## Audio Engine

### Hardware Init

Implement `audio_init()`:

- Enable sound with `REG_SNDSTAT = SSTAT_ENABLE`.
- Configure DMG channel routing and volume through libtonc sound control masks in `REG_SNDDMGCNT`.
- Keep Direct Sound disabled for MVP unless needed later.
- Initialize channel registers to a quiet state.

### Note Playback

Implement per-channel note triggers:

- Square channels write `REG_SND1CNT`/`REG_SND2CNT` and `REG_SND1FREQ`/`REG_SND2FREQ`.
- Wave channel writes wave RAM and `REG_SND3SEL`, `REG_SND3CNT`, `REG_SND3FREQ`.
- Noise channel writes `REG_SND4CNT` and `REG_SND4FREQ`.
- Use libtonc note/frequency helpers if suitable, such as `SND_RATE(note, oct)`.

Keep all hardware writes in `audio.c` so the sequencer does not directly touch sound registers.

### Timing

Use a deterministic tick system:

- VBlank IRQ drives UI refresh and input sampling.
- A timer IRQ or accumulated VBlank counter drives sequencer ticks.
- Start with VBlank-derived timing for simplicity.
- Move to timer IRQ timing if tempo precision is not acceptable.

Implementation path:

1. Initialize IRQs with libtonc IRQ helpers.
2. Use `VBlankIntrWait()` in the main loop.
3. Convert tempo into frames-per-row or fixed-point row advancement.
4. On each sequencer tick, process active row events and effects.

### Effects

MVP effects:

- Set tempo.
- Note cut.
- Volume set.
- Pattern break or jump.

Later effects:

- Arpeggio.
- Portamento.
- Vibrato.
- Duty change.
- Waveform select.

## UI Plan

### Video Mode

Use a tile/text UI for reliability and low CPU cost:

- Start with a libtonc text renderer if it is adequate.
- Otherwise use a simple 4bpp tile font copied with `dma3_cpy`.
- Use libtonc VRAM, screenblock, charblock, and palette symbols instead of raw addresses.

Recommended first layout:

- Top status row: song name placeholder, play/stop, tempo, pattern, row.
- Main grid: 4 tracks x visible rows.
- Bottom row: current edit mode, selected instrument, octave.

### Input

Use libtonc key helpers:

- D-pad: move cursor.
- A: enter/edit note or value.
- B: delete/clear cell.
- L/R: change track, octave, or edit field depending on mode.
- Start: play/stop.
- Select: switch edit mode.

Input handling should produce high-level editor actions rather than letting UI code poll raw key state everywhere.

## Data Flow

Main loop:

1. Wait for VBlank.
2. Poll/update input.
3. Dispatch editor actions.
4. Advance sequencer timing if playback is active.
5. Render dirty UI regions.

Playback:

1. `sequencer_tick()` checks whether a new row should fire.
2. For each track, read the current pattern cell.
3. Resolve instrument changes.
4. Apply note/effect events.
5. Call `audio_trigger_*()` or `audio_apply_*()` functions.

## Suggested Milestones

### Milestone 1: Project Skeleton

- Add `Makefile`.
- Add minimal `main.c`.
- Initialize display with libtonc.
- Print a static editor mockup.
- Build `agbseq.gba`.

Acceptance:

- ROM boots in an emulator.
- No custom GBA register definitions exist in project source.

### Milestone 2: Sound Bring-Up

- Initialize GBA sound through libtonc definitions.
- Play a test note on channel 1.
- Play test notes on channels 2, 3, and 4.
- Add a small note-to-register conversion wrapper.

Acceptance:

- Each hardware channel can be auditioned independently.
- All sound register writes are isolated to `audio.c`.

### Milestone 3: Sequencer Core

- Define song, pattern, cell, and instrument structures.
- Add a hard-coded demo pattern.
- Advance rows at a fixed tempo.
- Trigger notes on all four channels.

Acceptance:

- ROM plays a repeating multi-channel pattern.
- Tempo is stable enough for a first pass.

### Milestone 4: Pattern Editor

- Render the pattern grid.
- Add cursor movement.
- Add note entry, note deletion, instrument selection, and octave changes.
- Add play/stop control.

Acceptance:

- User can create or alter a short loop directly on the GBA.

### Milestone 5: Instruments and Effects

- Add square, wave, and noise instrument editors or compact controls.
- Add effect column editing.
- Implement MVP effects.

Acceptance:

- User can shape sounds enough to create recognizable patterns.

### Milestone 6: Persistence

- Add SRAM save/load for the current song.
- Add versioned save structure and checksum.
- Add fallback to demo song when save data is invalid.

Acceptance:

- Edited song survives reset on hardware/emulator with SRAM support.

### Milestone 7: Polish and Hardware Testing

- Test on mGBA and real hardware or flash cart if available.
- Verify timing, audio balance, and UI readability on a 240x160 display.
- Add bounds checks and clear failure behavior for invalid song data.

Acceptance:

- Sequencer is usable for composing short hardware-sound loops.

## Testing Strategy

### Host-Side Tests

Keep pure logic testable without GBA hardware:

- Song data validation.
- Pattern row advancement.
- Effect interpretation.
- Cursor movement rules.
- Save data checksum/version handling.

These tests can compile with a host C compiler if hardware-specific code is isolated behind small interfaces.

### GBA/Emulator Tests

Manual test ROMs or debug modes:

- Sound channel test.
- Sequencer timing test.
- Input repeat test.
- SRAM save/load test.
- UI grid redraw test.

Use emulator audio/video behavior as a fast check, but treat real hardware as the final audio reference.

## Risks and Mitigations

- GBA audio timing can drift if driven only by VBlank.
  - Start simple, then move sequencer timing to a timer IRQ if needed.
- Sound register programming is easy to scatter through the codebase.
  - Keep all direct sound register writes in `audio.c`.
- UI editing can become awkward with limited buttons.
  - Use explicit edit modes and keep shortcuts consistent.
- SRAM persistence can corrupt user work if versioning is loose.
  - Add save magic, version, size, and checksum before enabling writes.
- Direct Sound sample support can consume CPU and timers.
  - Defer until the hardware-channel sequencer is stable.

## Implementation Standards

- Prefer small C modules with narrow headers.
- Keep hardware-facing code isolated.
- Use fixed-size arrays for predictable memory use.
- Avoid dynamic allocation.
- Keep interrupt handlers short.
- Do not do expensive rendering outside VBlank-sensitive paths.
- Use libtonc names and helpers consistently.
- Document any raw numeric constant that remains, and replace it with a libtonc symbol when one exists.

## Open Questions

- Should the first version prioritize live performance controls or detailed pattern editing?
- Should the song format be intentionally compatible with an existing tracker format later?
- Is SRAM persistence required for the first playable build?
- Should Direct Sound sample playback be in scope after MVP, or should this remain a pure hardware-channel sequencer?
- Which emulator should be the primary fast feedback target?
