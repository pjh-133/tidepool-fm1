# The Blank Canvas's instruments

Generated from the firmware's sources by `tools/gen_skill.py`: do not edit by hand.

In recording mode, PRESETS turns through these 36 in this order (the number is its place). The keys play the chosen instrument, softer or louder with how hard they are pressed.

| # | Instrument |
|---|---|
| 1 | Glass bell |
| 2 | Kalimba |
| 3 | Felt keys |
| 4 | Pluck |
| 5 | Soft pad |
| 6 | Warm organ |
| 7 | Choir |
| 8 | Sub bass |
| 9 | Cello |
| 10 | Chip lead |
| 11 | Flute |
| 12 | Harp |
| 13 | Mellow brass |
| 14 | Marimba |
| 15 | Moog bass |
| 16 | Music box |
| 17 | Nylon guitar |
| 18 | Strings |
| 19 | Vibraphone |
| 20 | Drum kit |
| 21 | Hand drums |
| 22 | Rhodes |
| 23 | Harpsichord |
| 24 | Celesta |
| 25 | Toy piano |
| 26 | Koto |
| 27 | Oud |
| 28 | Shakuhachi |
| 29 | Steel pan |
| 30 | Violin |
| 31 | Muted trumpet |
| 32 | Soprano |
| 33 | Synth lead |
| 34 | Upright bass |
| 35 | Temple bell |
| 36 | Singing bowl |

Notes on a few of them:

- **Drum kit** and **Hand drums**: each key plays a different drum (below). OCT- and OCT+ do nothing for them (the screen says DRUMS: NONE, and KEYS shows KIT).
- **Upright bass** sounds an octave below the key, so it sits in the bass without pressing OCT-.
- Struck instruments (bells, plucks, keys, drums) ring out on their own; held ones (pads, organ, strings, winds, voices, leads) sound for as long as the key is held.

## Drum kit: which key plays what

Keys counted from the lowest (F3), black keys included: 27 keys, F3 to G5.

| Key | Note | Drum |
|---|---|---|
| 1 | F3 | kick |
| 2 | F#3 | rim (side stick) |
| 3 | G3 | snare |
| 4 | G#3 | clap |
| 5 | A3 | low tom |
| 6 | A#3 | closed hi-hat |
| 7 | B3 | mid tom |
| 8 | C4 | high tom |
| 9 | C#4 | pedal hi-hat |
| 10 | D4 | crash |
| 11 | D#4 | open hi-hat |
| 12 | E4 | ride |
| 13 | F4 | deep kick |
| 14 | F#4 | shaker |
| 15 | G4 | brush sweep |
| 16 | G#4 | tambourine |
| 17 | A4 | cowbell |
| 18 | A#4 | ride |
| 19 | B4 | kick |
| 20 | C5 | rim (side stick) |
| 21 | C#5 | snare |
| 22 | D5 | clap |
| 23 | D#5 | closed hi-hat |
| 24 | E5 | open hi-hat |
| 25 | F5 | low tom |
| 26 | F#5 | mid tom |
| 27 | G5 | high tom |

## Hand drums: which key plays what

| Key | Note | Drum |
|---|---|---|
| 1 | F3 | darbuka doum (deep centre) |
| 2 | F#3 | darbuka tek (bright rim) |
| 3 | G3 | darbuka ka (slap) |
| 4 | G#3 | riq (jingles) |
| 5 | A3 | frame drum |
| 6 | A#3 | frame drum slap |
| 7 | B3 | taiko |
| 8 | C4 | shime (small taiko) |
| 9 | C#4 | wood clappers |
| 10 | D4 | udu (clay pot) |
| 11 | D#4 | finger cymbals |
| 12 | E4 | agogo bell, low |
| 13 | F4 | agogo bell, high |
| 14 | F#4 | shaker |
| 15 | G4 | darbuka doum (deep centre) |
| 16 | G#4 | darbuka tek (bright rim) |
| 17 | A4 | darbuka ka (slap) |
| 18 | A#4 | riq (jingles) |
| 19 | B4 | frame drum |
| 20 | C5 | frame drum slap |
| 21 | C#5 | taiko |
| 22 | D5 | shime (small taiko) |
| 23 | D#5 | wood clappers |
| 24 | E5 | udu (clay pot) |
| 25 | F5 | finger cymbals |
| 26 | F#5 | agogo bell, low |
| 27 | G5 | agogo bell, high |
