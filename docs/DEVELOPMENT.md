# Tidepool on the FM-1: development notes

How the firmware is built, measured and tested. For playing it, see the [README](../README.md). Paths such as
`public/index.html` refer to the original Tidepool web app, which is not in this repository.

This repository holds a C port of Tidepool's five preset banks from the web app's `public/index.html` (Shallows, Deep
Space, Forest Floor, Neon Rain and Dungeon Synth), written the way an FM-1 firmware engine is written. It uses single-precision
float with no maths library, and a fixed pool of voices instead of fresh Web Audio nodes for every note. Effects and
the compressor are written as plain DSP. The same engine runs in three places: a Mac benchmark (`host/`), a browser
emulator (`web/`), and an installable FM-1 firmware (`firmware/`).

| Source | What |
|---|---|
| `src/tp_engine.c` | The engine: voices, loops, scheduler, reverb, delay, compressor |
| `src/bank_*.c` | The banks, each a line-by-line port of its `BANKn_LOOPS` in the web app |
| `src/banks.c` | The five banks in order (`TP_BANKS`) |

## Build and run

```
./build.sh
./build/tp_bench all 90 32 out.wav     # all 16 loops, 90 s, 32-voice limit
./build/tp_bench solo                  # each loop's cost and voice count
./build/tp_bench build                 # loops joining one by one
./build/tp_bench sweep                 # voice limits 8..192: what gets stolen, what it costs
./build/tp_bench mask 0200 30 k.wav    # chosen loops (bit n = loop n), e.g. Kalimba alone
./build/tp_bench tweak                 # each loop setting at its extremes, on every loop
TP_BANK=4 ./build/tp_bench all 90 32   # any mode, another bank (1 Shallows .. 5 Dungeon Synth)
```

## The banks

All 16 loops of each bank, 72 bpm, no voice limit:

| Bank | Peak voices | Mean voices | FM-1 estimate |
|---|---|---|---|
| Shallows | 30 | 21.6 | 49 % |
| Deep Space | 26 | 15.9 | 45 % |
| Forest Floor | 27 | 15.7 | 39 % |
| Neon Rain | 22 | 13.8 | 40 % |
| Dungeon Synth | 28 | 17.2 | 50 % |

None needs more than 32 voices, so nothing is stolen on the device.

Banks 2–5 needed four engine features beyond Shallows':

| Feature | Used by |
|---|---|
| Filter sweeps (`tv_filter_ramp`: linear or exponential, up to three stages) | Nebula pad, Pulse arp, Chord stabs, Pluck arp, Traffic |
| A tremolo after the envelope (`tv_amp`, `LFO_AMP`) | Stone organ, Cold strings |
| Two filters side by side (`tv_filter_parallel`) | Lost choir's formants |
| Chords split across voices with identical filters | Nebula pad and Juno pad (10 saws), Cold strings (9), Tolling bell (7 partials) |

Two linear pitch glides (Hull creak, Whistle echo) are exponential here; over a few semitones the two are the same.

Switching banks (`tp_stop_fast`, `tp_quiet`, `tp_set_bank`) works as the web app's: every loop fades out in 0.35 s,
then the new bank starts on its first chord, while the reverb and echo ring on.

## Browser emulator (`web/`)

The same engine sources are compiled to WebAssembly (23 KB, with no imports) and run in an AudioWorklet at 44.1 kHz.
The page is a to-scale FM-1 front panel with all 27 keys, 14 buttons, 7 encoders, the MASTER pot and a 240 × 240
screen, using the firmware's control scheme. It still plays Shallows only: the built `web/tp.wasm` predates the
other banks, and rebuilding it needs wasi-sdk (below).

| Control | Does |
|---|---|
| White keys 1–16 | Toggle the loops (rows: Drones, Textures, Melodies, Beats). Lit while playing, blinking while joining |
| KNOB1–4 | The selected setting for the selected row's four loops |
| OCT− / OCT+ | Cycle the setting: Volume, Attack, Decay, Reverb, Echo, Pan (bottom left of the screen) |
| ALGORITHM | Pick the row (pressing a key also picks its row) |
| SELECT | Tempo |
| MASTER | Main volume |
| PLAY/STOP | Stop everything; press again for the same mix |
| HOME (hold 3 s) | Surprise me: a countdown on the screen; let go early and nothing happens |
| PRESETS | Bank: only Shallows is ported |
| REC, black keys and other buttons | Unassigned |

Under the panel are live engine counts (voices, peaks, stolen notes) and a voice-limit selector.

To serve it:

```
python3 -m http.server 8790 --bind 127.0.0.1 --directory web   # then open http://localhost:8790
```

To rebuild `web/tp.wasm`, use [wasi-sdk](https://github.com/WebAssembly/wasi-sdk/releases). Unpack it anywhere; it
installs nothing.

```
WASI_SDK=/path/to/wasi-sdk-34.0-arm64-macos web/build.sh
```

The page's CPU figure is not measured in the browser. It is the effects' 10.8 % plus each sounding loop's cost
from `tp_bench solo`, so it holds for no voice limit; a lower limit costs less.

## The installable firmware (`firmware/`)

An FM-1 firmware: thirteen preset banks (the web app's five, then eight written for the FM-1) and a Blank Canvas, on the
engine in `src/`, played from the panel the way the browser emulator plays it. It is built on [fm1-base](https://github.com/charlesvestal/fm1-base) (in `base/`, MPL-2.0, with
felucca-bsp inside it), which supplies the hardware layer, screen drawing, saving, USB and the update package. The
app is `firmware/app.c`.

| Control | Does |
|---|---|
| White keys 1–16 | Toggle the loops (rows: Drones, Melodies, Beats, then Textures, the scene's treatments, last). Lit while playing, blinking while joining |
| KNOB1–4 | The selected setting for the selected row's four loops |
| OCT− / OCT+ | Cycle the setting: Volume, Attack, Decay, Reverb, Echo, Pan (bottom left of the screen) |
| ALGORITHM | Pick the row (pressing a key also picks its row) |
| SELECT | Tempo |
| MASTER | Main volume |
| PLAY/STOP | Stop everything; press again for the same mix (green while playing). It acts when let go |
| PLAY held + KNOB1 | The bank's chord progression: 0 its own, 1–99 new ones in its key, from the next bar line. Held a moment, PLAY shows "KNOB 1: CHORDS" and outlines the chords until let go |
| HOME (hold 3 s) | Surprise me: a countdown on the screen; let go early and nothing happens |
| SEQ | Autoplay, on any bank (the canvas: its recorded loops). Once every four bars (so faster at a higher tempo) it brings a loop in or out, aiming for two in each of the top three rows and one texture: from nothing, a loop a cycle, every row first, so seven loops in seven cycles (about 1½ minutes at 72 bpm); then mostly a swap within a row, now and then one out for a cycle, sometimes nothing; never a row left empty, the longest playing out first. The textures row (each bank's second family: Textures, Atmosphere, Woods, City, Depths) has one loop at a time, changed only after 64 bars and then about one cycle in four (every few minutes). The presets' chord progression changes every 16 bars or so. SEQ lit and AUTOPLAY in the footer while on. SEQ again ends it: once every four bars, fewer loops (two at a time while more than four play), the fullest row first, the beat before the tunes, the tunes before the texture, the drones last, until everything has stopped (SEQ blinking, ENDING in the footer; SEQ during the ending goes back to autoplay). PLAY stops everything at once |
| (textures) | Every texture loop starts at 25 % volume (the others 70 %); settings saved before keep any changed by hand, and those left at 70 % go to 25 %. Surprise gives them 15–35 % |
| SAVE | Store the tempo, the bank, and every loop's settings in all five banks in flash; lit when there are unsaved changes |
| PRESETS | The bank (the five presets, then the Blank Canvas). It changes once the knob rests (0.7 s): everything fades out, then the new bank is ready |
| GLO, REC, EDIT | The Blank Canvas's (below); in a preset bank the screen says so |
| Black keys and other buttons | Unassigned: the screen says so |

Active pools show their names in white, with a dark shadow so they stay readable on the brightest colours.

**Chord progressions (`src/tp_harmony.c`).** Hold PLAY and turn KNOB1: progression 0 is the bank's own, 1 to 99
are new ones, each the same every time (`TP_BANK=1 ./build/tp_bench chords 12` lists them). A progression keeps the
bank's key and mode (Shallows D minor, Deep Space E minor, Forest Floor G major, Neon Rain and Dungeon Synth A
minor), starts on the home chord and moves by functional harmony's pulls, the last chord leaning home, with no
diminished chord, no repeat and at most one suspended chord. Its chords take the bank's flavour (7ths, 9ths and 6/9s;
add9s and sus4s; or plain triads), voiced to move as little as possible, in the bank's register. The loops that read
the chord (pads, arps, bass lines, choirs, strings) follow it; the note grid (Kalimba, Marimba, Harpsichord) keeps its
rhythm, each note moved to the nearest scale note in the chord under it; drones and the pentatonic melodies stay in
the key. A change starts at the next bar line. Each bank keeps its progression, and SAVE stores them (settings saved
before this, format "TDP2", are kept). Progression 0 renders bit for bit as before; `ab/chords/` has one new
progression of each bank.

**The look (`firmware/art.c`).** The start shows a pixel-art splash, a tide pool at dusk, animated for 2.6 s (any
key, button or knob goes straight on). Each bank then has its own look, after the web app's themes: its palette, an
animated banner behind its name in pixel letters, and the shape of its pools.

| Bank | Banner | Pools |
|---|---|---|
| Shallows | water: light shafts, ripples, rising bubbles | rounded, a waterline moving along the top |
| Deep Space | twinkling stars, now and then a shooting star | orbs |
| Forest Floor | pines against the dusk, fireflies blinking | pebbles, each a different shape |
| Neon Rain | a skyline with lit windows, slanting rain | neon tubes: bright outlines when playing |
| Dungeon Synth | stone courses in wavering torchlight, embers rising | arches; stone courses when idle |
| Aurora | the northern lights rippling over snowy hills, stars | ice: cut corners like facets |
| Dunes | dusk with a crescent moon, layered dunes, a camel caravan crossing | a dune's slope along the top |
| Zen Garden | a snow-capped mountain, a blossom branch, petals falling, raked gravel | smooth stones, raked lines when idle |
| Clockwork | two gears turning against each other, a pendulum swinging | gear teeth along the edges |
| Night Café | a cup of coffee on its saucer, steam rising, a string of lights | a café booth: rounded top, square foot |
| Bonfire | ruined towers against an ashen dusk, a bonfire with a sword thrust in it, embers rising | heater shields |
| Old Blood | gothic spires under a great moon that slowly turns blood red and back, lit windows (one guttering, each going dark now and then), fog rolling across the roofs, a crow crossing | lancet windows: a pointed top |
| Cyberpunk | towers in an orange smog, a billboard high on the tallest turning pink to blue, fire bursting from two stacks, a flying car crossing, rain | a display's frame: two corners cut on the slant |

All of it is drawn procedurally into fm1-base's canvas in the main loop, the banner about ten times a second (4 ms of
the screen's time each).

**The Blank Canvas (`src/tp_canvas.h`, `bank_canvas.c`, `tp_instruments.c`).** The sixth bank: 16 empty loops to
record into, on paper with drifting watercolours. It has two modes, switched by tapping GLO.

| Mode | Controls |
|---|---|
| HOME (recording mode) | Back to playback, as GLO (a take running is cancelled) |
| Playback | As a preset bank: the keys toggle the recorded loops (an empty one says "EMPTY - GLO TO RECORD"); the setting cycle adds INSTRUMENT, the loop's own. No chord progressions and no surprise |
| Recording (GLO lit) | The keys play the instrument with velocity (fm1-base's `lib/touch`); OCT−/OCT+ shift the keys' octave; PRESETS picks the instrument; KNOB1 sets a new loop's length (1, 2 or 4 bars), KNOB2 its snap (off, 1/16, 1/8, or BAR: each note starts on its bar line and lasts whole bars, for held chords; snapping is applied as the loop plays, so what you played is kept), KNOB3 the click; PLAY stops and starts the mix. The screen shows these, then the loop's bars with its notes and the playhead |
| REC (recording mode) | 1: a new loop in the next free slot. 2: armed, a bar's count-in from the next bar line. 3: done. The take loops round its length, each pass adding to the last, until REC. It is saved at once (no SAVE needed) and plays on if anything else is playing; otherwise PLAY brings it in. REC during the count-in, or GLO during a take, cancels |
| EDIT held (either mode) | The loops: tap a key to pick one (its frame blinks red), the same key again deletes it (saved at once), any other key cancels |

The 36 instruments: first the web app's 19 melodic ones (Glass bell to Vibraphone), with its envelopes, filters, FM
and velocity response; then 17 written for the FM-1 (`src/tp_instruments.c`), each its own function, most from the
preset banks:

| Kind | Instruments |
|---|---|
| Drums: a sound a key, the octave ignored (the screen's KEYS shows KIT) | Drum kit (F3 kick, F#3 rim, G3 snare, G#3 clap, A3 low tom, A#3 closed hat, B3 mid tom, C4 high tom, C#4 pedal hat, D4 crash, D#4 open hat, E4 ride, F4 deep kick, F#4 shaker, G4 brush, G#4 tambourine, A4 cowbell, A#4 ride, then the main ones again); Hand drums (darbuka doum, tek and ka, riq, frame drum and slap, taiko, shime, wood clappers, udu, finger cymbals, two agogo bells, shaker, then again) |
| Keys | Rhodes, Harpsichord, Celesta, Toy piano |
| World | Koto, Oud, Shakuhachi, Steel pan |
| Leads and voices | Violin, Muted trumpet, Soprano, Synth lead |
| Bass and bells | Upright bass (an octave below the key), Temple bell, Singing bowl |

Each was levelled against the web app's 19 (a note at velocity 100). Loops recorded before keep their instruments:
the new ones come after the 19. `firmware/tests/instruments.txt` plays the kits and records a beat, then plays each
instrument (`firmware/build/host/instruments.wav`). Each loop is its own flash object (16 of them, in the data area from 0x97000); SAVE also stores an
instrument changed with the knobs, the recorder's settings and the bank. Settings saved before this (format "TDP3" or
"TDP2") are kept. `firmware/tests/canvas.txt` records, plays, reboots, overdubs on a playing loop, deletes and cancels;
`host/tp_canvas_test.c` checks the recorder's timing in the engine (`ab/canvas/recorded_take.wav`).

**The FM-1's own banks (`src/bank_aurora.c` … `bank_oldblood.c`).** Seven more banks, after Dungeon Synth and before the
Blank Canvas, written for the FM-1 in the same style as the web app's: 16 loops each, four rows (drones, textures,
melodies, beats; the textures last on the keys), their own key and chords, so PLAY + KNOB1 and autoplay work in them.

| Bank | Key, chords | Drones | Textures | Melodies | Beats |
|---|---|---|---|---|---|
| Aurora | D major: Dmaj9 Bm9 Gmaj7 Aadd9 | Polar drone, Aurora pad, Glass bow, Low swell | Blizzard, Ice cracks, Whale song, Snow hiss | Ice bells, Celesta, Shimmer arp, Spirit flute | Heart drum, Sleigh bells, Ice tick, Deep boom |
| Dunes | C minor: Cm Fm Bbsus4 Ab | Tanpura, Night pad, Low strings, Dune swell | Sand wind, Campfire, Camel bells, Crickets | Oud, Ney flute, Santur, Distant voice | Darbuka, Frame drum, Finger cymbals, Riq jingles |
| Zen Garden | F major: Fadd9 Am7 Bbadd9 Csus4 | Temple drone, Sho, Singing bowl, Low koto | Bamboo fountain, Rain on leaves, Wind chimes, Cicadas | Koto, Shakuhachi, Temple bell, Water drops | Taiko, Wood clappers, Small bell, Sand rake |
| Clockwork | G minor: Gm Ebmaj7 Cm7 Dm7 | Steam hum, Reed organ, Musical saw, Low gears | Gears, Steam hiss, Cuckoo, Old radio | Music box, Celesta, Toy piano, Clarinet | Tick tock, Ratchet, Hammer, Pendulum |
| Night Café (lofi) | Bb major, swung sixteenths: Bbmaj9 Ebmaj9 Dm7 Gm9 | Dusty Rhodes, Sub bass, Tape pad, Lazy piano | Vinyl crackle, Rain on window, Murmur, Espresso | Sleepy piano, Mellow guitar, Flute, Music box | Boom kick, Dusty snare (laid back), Lazy hats, Snaps |
| Bonfire (in the mood of Dark Souls) | D minor: Dm Bbmaj7 Gm Asus4 | Ash drone, Mournful strings, Cathedral organ, Low choir | Bonfire, Ashen wind, Distant bell, Chains | Harp, Solo cello, Soul glimmer, Lone soprano | Timpani, Anvil, Heavy steps, Sword scrape |
| Old Blood (in the mood of Bloodborne) | F minor, C major for the harmonic minor: Fm Db Bbm C | Dread drone, Gothic choir, Pipe organ, Tremolo strings | Night fog, Church bells, Crows, Rain on cobbles | Lullaby box, Violin lament, Harpsichord, Eerie voice | Low toms (3+3+2), Coffin knock, Cello stabs, Bone rattle |
| Cyberpunk (in the mood of Blade Runner) | E minor: Em9 Cmaj7 Am9 B7sus4 | Brass swell, Low drone, Glass pad, Dark choir | Acid rain, Flying car, Street voice, Far sirens | Lonely sax, Tears piano, Bells, Bleeps | Big drum, Metal hit, Seq bass, Ticks |

Bonfire, Old Blood and Cyberpunk are written in the mood of those games and that film; every sound, chord and picture is the FM-1's own, with
nothing taken from the games' music or art.

Each loop was levelled against the loops doing the same job in the web app's banks (alone, 24 s), and each bank's
whole mix against theirs: all thirteen within -19 to -27 dBFS with every loop on. On the Mac's estimate they cost what
the web app's do (27-38 % with all 16 loops, the five originals 29-39 %), none stealing a note. Settings saved by
earlier firmwares (the twelve-bank "TDP7", the ten-bank "TDP6", the five-bank "TDP5" or older) are kept: every bank's, the canvas's (now
the fourteenth bank), the recorder's and the tempo; `firmware/tests/migrate.txt` checks it. `firmware/tests/newbanks.txt` plays each new
bank with every loop on, its screen and a recording (in `firmware/build/host`).

Each bank keeps its own loop settings. The screen shows the bank's name, its chords and its row names. Settings
saved by the Shallows-only build (format "TDP1") are not read; the first start of this build begins from the
defaults.

The footer shows the FM-1's own measured audio load (`plat_cpu_pct`) and any audio dropouts ("XRUN") since start-up.

Build results (fm1-base's checks all pass):

| Check | Result |
|---|---|
| App image | 147 KB (slot about 568 KB) |
| RAM | 60 KB of 96 KB (31 KB of it the canvas's loops) |
| Pool | 324 KB of 336 KB (the engine's buffers and fm1-base's screen canvas) |
| Package | `firmware/build/tidepool.fwsc`, identity FM-1_8600 |

Not in this build: Bluetooth (it needs JieLi's full SDK), the second core, MIDI handling beyond what fm1-base does
itself, drum kits for the canvas, overdubbing an existing canvas loop, undo, and syncing loops with the web app.

### Build and test

The host simulator runs the whole app on the Mac (no toolchain needed): keys, knobs, screen, sound and saving.

```
python3 base/tools/build.py firmware --host
firmware/build/host/tidepool_host firmware/tests/basic.txt firmware/build/host   # screenshots and a WAV there
firmware/build/host/tidepool_host firmware/tests/canvas.txt firmware/build/host  # the Blank Canvas
firmware/build/host/tidepool_host firmware/tests/instruments.txt firmware/build/host  # its 36 instruments
python3 tools/gen_skill.py --check   # the Claude skill's bank and instrument lists match the sources
firmware/build/host/tidepool_host firmware/tests/autoplay.txt firmware/build/host  # autoplay (SEQ)
firmware/build/host/tidepool_host firmware/tests/newbanks.txt firmware/build/host  # the FM-1's own five banks
firmware/build/host/tidepool_host firmware/tests/migrate.txt firmware/build/host   # older settings kept
```

The firmware itself needs Docker running, JieLi's toolchain at `~/.jieli/toolchain`, and three files of JieLi's
AC79 SDK under `~/fw-AC79_AIoT_SDK/cpu/wl82/tools/` (`uboot.boot`, `cfg_tool.bin`, `cfg/eq_cfg_hw.bin`; the build
checks them against fm1-base's pinned hashes):

```
AC79_SDK=~/fw-AC79_AIoT_SDK JIELI_TOOLCHAIN=~/.jieli/toolchain python3 base/tools/build.py firmware
```

### Installing

Installing is at your own risk. fm1-base's installer works over USB-MIDI from the stock firmware or from another
fm1-base or Felucca firmware. It needs `pip3 install mido python-rtmidi`.

```
python3 base/bsp/tools/fm1_install.py --info                       # which FM-1, and what it runs
python3 base/bsp/tools/fm1_install.py firmware/build/tidepool.fwsc
python3 base/bsp/tools/fm1_install.py FM-1.fwsc                    # back to stock: M-VAVE's official V15 file
```

If a firmware ever fails to start, felucca-bsp's `base/bsp/tools/fm1_rescue.sh` puts the stock firmware back through
the chip's own update mode. It backs up the whole flash first and never touches the bootloader. Don't install over
SLOOP-based firmwares: an install over SLOOP has bricked an FM-1 (ChoralRoot's notes).

## Built for the FM-1's chip (`fm1-build.sh`)

The engine compiles cleanly with JieLi's own compiler (clang 4.0.1 for pi32v2) and the flags fm1-base uses for an
app's DSP (`-O2 -mcpu=r3 -mfprev1 -ffp-contract=off`), with a 32-voice pool:

| Check | Result |
|---|---|
| Code | 35.8 KB, plus 4.4 KB of constants, for the engine and all five banks (the app slot is about 568 KB) |
| RAM | 211 KB: delay 144 KB, reverb 43 KB, voices 11 KB, tables and buses |
| Needs from outside | `memset` only (fm1-base's `bsp/chip/libc.c` has it) |
| Double-precision maths | None; every float operation is an FPU instruction, with no software maths routines |

The inner loops compile to about as many instructions as on the Mac:

| Loop | FM-1 build | Mac build |
|---|---|---|
| Sine oscillator | 19 | 21 |
| Filter | 22 | 19 |
| White noise | 14 | 15 |
| FM sine | 51 | about 45–51 |
| Whole renderer | 1,517 | 1,554 |

So the Mac instruction counts behind the CPU estimate are a fair stand-in for this code. How many cycles each FPU
instruction takes on the FM-1 is still unknown; only the device's CPU meter can tell.

```
./fm1-build.sh        # needs Docker running and JieLi's toolchain at ~/.jieli/toolchain
```

## Measured on the FM-1 (`tools/fm1_bench.py`)

The firmware times its own render (`TP_PROFILE`: the engine's stages, with fm1-base's 24 MHz clock) and answers
over USB-MIDI SysEx, so the Mac can measure the device directly. The FM-1 plays out loud while it runs.

```
python3 tools/fm1_bench.py           # idle, then each bank with all 16 loops on (needs mido, python-rtmidi)
python3 tools/fm1_bench.py --solo 3  # one bank, each loop on its own
```

All 16 loops, 72 bpm, % of the render's time (average, and the worst block in brackets):

| Bank | First build | Optimised | Voices | Mixing | Effects |
|---|---|---|---|---|---|
| Idle | 16 % | 12 % | 0.6 % | 0.6 % | 9.6 % |
| Shallows | 84 % (98 %) | 66 % (80 %) | 44 % | 11 % | 9.6 % |
| Deep Space | 80 % (101 %) | 59 % (77 %) | 37 % | 10 % | 9.6 % |
| Forest Floor | 68 % (89 %) | 48 % (68 %) | 28 % | 9 % | 9.6 % |
| Neon Rain | 71 % (88 %) | 51 % (65 %) | 31 % | 9 % | 9.6 % |
| Dungeon Synth | 91 % (113 %) | 70 % (92 %) | 49 % | 10 % | 9.6 % |

The first build dropped audio in Dungeon Synth (721 dropouts in 15 s); the optimised build drops none.

**The overload guard.** fm1-base calls `app_overload` when a block takes over 90 % of its time; the engine then
fades its two quietest notes out in 3 ms (`tp_shed`), keeping the loops' continuous sounds. A shorter tail instead of
a dropout. The footer counts them ("SHED"), and `fm1_bench.py --stress 5` overloads Dungeon Synth on purpose (every
loop's attack and decay at maximum, not saved) to watch it work. Notes also end once their decay reaches -80 dB
instead of rendering silence until their stop (the output changes by 65-74 dB below the mix), and each note has its
own noise generator, so one note's length never changes another's noise or the loops' random choices. fm1-base's own
meter (USB audio, the key scan) adds next to nothing: it reads what the render takes.

The Mac estimate below undershoots the device by about 1.7x: the FM-1's FPU costs more per instruction than the
fixed-point code Felucca's ratio was measured on, and the mixing and effects, cheap on the Mac, were a third of the
first build's time. The optimisations, each checked against the earlier output (the same level to 0.01 dB, the
difference 46-54 dB below the mix):
- the mixer skips loops with nothing sounding in a block, and keeps the per-sample work to a multiply and four or six
  adds;
- envelopes run at control rate (32 samples), interpolated;
- oscillators have loops without per-sample glides or decays for steady notes, and no division in the saw and square
  anti-aliasing;
- the reverb's lines are float (no conversion or clamp per sample), and a 1e-18 offset keeps the effects' tails out
  of the denormals.

## How the CPU is estimated

The FM-1 firmwares can't be timed without the hardware, so this uses Felucca's calibration:

1. Count the instructions the Mac's kernel reports per output sample, with `cc -O2 -ffp-contract=off`.
   Auto-vectorisation is also switched off, because the FM-1's core has no SIMD.
2. Multiply by **1.7 % of the FM-1 per 100 instructions per sample**. That ratio comes from on-device
   measurements of Felucca's PHYS engine (`tests/drum_test.c` and `tests/fm6_test.c` in Felucca).

For comparison, the same counter gives:

| What | Instructions per sample | FM-1 estimate |
|---|---|---|
| AMB-1's engine (ships on the FM-1), default pieces | 1,221–1,390 | 21–24 % |
| AMB-1, all layers turned up | 1,493 | 25 % |
| Felucca's heaviest mix (3 parts plus drums) | 1,574 | 27 % |
| **Shallows, all 16 loops (this port)** | **2,869** | **49 %** |
| Shallows, all 16 loops, 16-voice limit | 2,000 | 34 % |

## Results (72 bpm, all 16 loops)

**Voices:**
- 30 at peak and 21.6 on average, with nothing stolen at a 32-voice limit.
- The web app has about 145 sounds running at once (measured). The difference comes from
  merging each chord into one voice, since all its notes share a filter and an envelope.

**Memory, about 208 KB of the ~360 KB pool:**

| Part | Size |
|---|---|
| Delay (int16, 0.8 s stereo for 56 bpm) | 144 KB |
| Reverb (8-line FDN, int16) | 43 KB |
| 32 voices | 10 KB |
| Sine table | 4 KB |
| Diffusers | 4 KB |

**Code:** about 19 KB (Mac `-Os`; the FM-1 code slot is 568 KB).

**Cost by loop** (FM-1 %, on top of 10.8 % for the effects):

| Loop | FM-1 % | Loop | FM-1 % | Loop | FM-1 % | Loop | FM-1 % |
|---|---|---|---|---|---|---|---|
| Shimmer | 5.9 | Tape choir | 4.9 | Glass arp | 4.6 | Air pad | 4.1 |
| Low drone | 2.5 | Wind | 2.4 | Kalimba | 2.2 | Rain | 2.0 |
| Bass line | 1.8 | Sub pulse | 1.5 | Ocean | 1.4 | Vinyl crackle | 1.3 |
| Brush snare | 0.9 | Brushed hats | 0.9 | Soft kick | 0.8 | Rim clicks | 0.7 |

**Voice limit sweep:**

| Limit | Notes stolen | FM-1 estimate |
|---|---|---|
| 8 (Felucca's) | 76 % | 27 % |
| 12 | 40 % | 30 % |
| 16 | 25 % | 34 % |
| 24 | 5 % | 44 % |
| 32 | 0 % | 49 % |

The short noise bursts (rain, crackle, hats) each count as a voice. A device engine would give them a
cheaper pool of their own.

## Loop settings

Every loop has the web app's loop settings: volume, attack, decay, reverb, echo and pan (`tp_param`). They use the
same ranges as the web app's sliders, and the defaults are each preset loop's designed sound. `tp_bench tweak` sets
every loop to each extreme:

| Every loop set to | Peak voices | FM-1 estimate |
|---|---|---|
| Defaults | 29 | 49 % |
| Attack at maximum (+2.5 s) | 68 | 89 % |
| Decay at maximum (4×) | 60 | 92 % |
| Decay at minimum (¼×) | 27 | 42 % |
| Reverb, echo or pan at either extreme | 29 | 48–51 % |

The sends and pan are free: the mixer applies them every block whatever their value. Long attacks and decays keep
notes sounding, so voices pile up. Attack hits hardest, because it turns every short hit (rain drops, hats) into a
2.5 s swell. The web app greys out Attack and Decay on drum instruments for recorded loops. On the device, the voice
limit is the safety net: it caps the load by stealing the oldest notes.

## How close it sounds (the `ab/` folder)

The `web_*.wav` files are the web app's real output, recorded in Chrome at 48 kHz. The `port_*.wav` files are
this engine at 44.1 kHz.

Each bank's full 16-loop mix against the web app's:

| Bank | Loudness | Octave bands, 31 Hz – 8 kHz |
|---|---|---|
| Shallows | −0.2 dB | within ±1.8 dB |
| Deep Space | +0.4 dB | within ±1.2 dB |
| Forest Floor | −0.8 dB | within ±2.2 dB |
| Neon Rain | −0.4 dB | within ±1.0 dB |
| Dungeon Synth | −0.1 dB | within ±2.0 dB |

Ten solo loops (four from Shallows, six from Neon Rain) are within −0.4 to +1.1 dB. Glass arp's note envelopes
correlate at 0.96 with the web app's.

Two things the comparisons found and fixed:
- **Chromium's sawtooth and square are 1.45 dB below ideal waves**, scaled down to remove their overshoot (measured
  with an OfflineAudioContext). The engine scales its own to match.
- **Every wave starts as Chromium's does**, at zero and rising, in step with a sine. A triangle and a sine on the
  same note (Neon Rain's Dark sub, Dungeon Synth's Lute) otherwise sat a quarter cycle apart and partly cancelled.

The compressor's makeup gain (1.10) is the best fit over the ten solo loops and five full mixes.

`port_all_16voices.wav` is the same mix with a 16-voice limit, so you can hear what voice stealing does. Randomness (rain, crackle, ghost
notes) and the start position in the 64-step cycle differ between recordings, so the waveforms are not identical.

Known differences:
- **Reverb.** The web app convolves with 4.5 s of decaying noise. Here an algorithmic FDN reverb is set to the
  same energy (`tp_bench rvcal`), with a 3.6 s decay. It is close, but not the same tail.
- **Compressor.** It runs at control rate on each block's peak and uses Chromium's knee shape. Its makeup gain
  was measured against the web app. Chromium's look-ahead and adaptive release are not copied.
- **Waveforms.** Triangle waves are naive, so they alias slightly above 8 kHz, about 60 dB down. Saw and square
  waves use polyBLEP.
- **Voice stealing** is a hard cut. A device engine would fade a stolen voice over a few milliseconds.

## Not covered here

- Instruction counts on the Mac are a proxy. The 1.7 % ratio was calibrated on fixed-point code, and float on
  the FM-1's FPU may cost a different amount per instruction. The firmware's footer shows the device's real load.
- The Blank Canvas banks: user loops, recording, patterns, performances and MIDI.

## Licence note

The engine (`src/`), the emulator (`web/`) and the app (`firmware/app.c`) are written from scratch, with no Felucca,
AMB-1 or X0X code. They only borrow methods (the calibration, the block structure). The firmware is built on
fm1-base and felucca-bsp, both MPL-2.0: their files stay MPL-2.0 (changes to them would be published), and the app
and engine may use any licence. `firmware/fm1.toml` says "proprietary" for now; change it if you publish.

## Publishing a release and the website

The website (<https://pjh-133.github.io/tidepool-fm1/>: front page, web installer, manual, the firmware in the
browser, and the `.fwsc` download) is made by fm1-base's site tools from `firmware/site/landing.md`,
`firmware/docs/MANUAL.md` (the README's "The scenes" to "The 36 instruments", with `img/` paths: keep the two in step)
and a release build. It needs the JieLi toolchain and SDK, Emscripten (`brew install emscripten`) and a Python with
Pillow. One command builds release VERSION, its browser version and the site, and pushes the site to the `gh-pages`
branch, which GitHub Pages serves:

```
AC79_SDK=~/.jieli/ac79-sdk JIELI_TOOLCHAIN=~/.jieli/toolchain base/tools/publish_site.sh firmware 1.0.1
```

A release's identity is `FM-1_86` and the version's six digits (1.0.0: `FM-1_86010000`); the installer checks it after
writing. Settings and recorded loops are kept across releases.

## The Claude skill (`plugin/`)

`plugin/skills/tidepool-fm1/` is a Claude skill that answers questions about using Tidepool: `SKILL.md` (when to use
it, how to answer, the essentials) and `references/` (controls, banks, the Blank Canvas, instruments,
troubleshooting). `plugin/.claude-plugin/plugin.json` makes it a Claude Code plugin, and `.claude-plugin/marketplace.json`
at the top lets Claude Code add this repository as a marketplace (`claude plugin validate .` checks both).

`references/banks.md` and `references/instruments.md` are generated from the sources by `tools/gen_skill.py` (a new
bank also needs its line in the script's `MOOD`). Run it after changing a bank, a loop's name or an instrument;
`tools/gen_skill.py --check` fails while they are out of date. The other references are written by hand: keep them
in step with a change to the controls or the screen's messages, and bump `version` in `plugin.json`.

For Claude.ai, the skill goes up as a zip attached to each GitHub release (the README links to the latest):

```
(cd plugin/skills && zip -qr -X ../../build/tidepool-fm1-skill.zip tidepool-fm1 -x '*.DS_Store')
gh release create v1.0.1 firmware/build/tidepool-1.0.1.fwsc build/tidepool-fm1-skill.zip
```
