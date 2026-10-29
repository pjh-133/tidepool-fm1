# The Blank Canvas: recording loops

The last bank (turn PRESETS all the way). It starts with sixteen empty pools; each fills with a loop the player
records, with its own instrument. It has two modes.

## Playback mode (normal)

The canvas plays like any bank: white keys 1-16 turn recorded loops on and off (an empty one says EMPTY - GLO TO
RECORD), the knobs adjust them, SEQ autoplays them, PLAY stops and starts. OCT- / OCT+ have a seventh setting,
INSTRUMENT: the knobs then change a recorded loop's instrument (the notes stay the same). Press SAVE to keep an
instrument changed that way. There's no chord progression and no surprise on the canvas: its loops play their own
notes.

## Recording mode (GLO)

Press **GLO**: GLO lights and the screen shows the recorder. The keys now play the chosen instrument, all 27 of
them (black keys too), softer or louder with how hard they're pressed. PLAY still starts and stops the mix, so the
player can record over loops that are playing.

The recorder's screen shows, from the top: the instrument; a row of four settings (LENGTH, SNAP, CLICK, KEYS); the
loop being made, its bars with the notes in it and a playhead; and a footer saying what REC does next.

| Control (recording mode) | What it does |
|---|---|
| PRESETS | Choose the instrument (36 of them, `instruments.md`) |
| KNOB1 | A new loop's length: 1, 2 or 4 bars |
| KNOB2 | Snap: OFF (as played), 1/16, 1/8, or BAR (each note starts on its bar line and lasts whole bars: for holding chords). Snapping is applied as the loop plays back, so what was played is kept |
| KNOB3 | The click during the count-in and take: ON or OFF |
| KNOB4 | Not used (KNOB 4 NOT USED HERE) |
| OCT- / OCT+ | The keys' octave, -2 to +2. KEYS shows the lowest key's note (F3 normally). For the drum kits it says DRUMS: NONE and KEYS shows KIT |
| REC | Record (below) |
| EDIT (held) + a key | Delete a loop (below) |
| GLO or HOME | Back to playback mode |

LENGTH, SNAP and CLICK can't be changed while a take is counting in or recording (AFTER THE TAKE REC: DONE).

### Recording a loop: REC three times

1. **REC** (first press): a new loop is set up in the next free pool. The screen says NEW LOOP n, REC AGAIN:
   COUNT-IN. Set the instrument, length, snap and click now if not already.
2. **REC** (second press): armed. The count-in starts at the next bar line and lasts one bar (four beats, with the
   click if it's on). COUNT-IN FROM THE NEXT BAR. Pressing REC during the count-in cancels (CANCELLED).
3. Recording starts on the bar after the count-in: RECORDING, REC: DONE GLO: CANCEL. Play. The loop goes round and
   round its length; each pass **adds** to what's already there and plays it back, so a loop can be built up in
   layers (a bass line on the first pass, then chords, then a melody).
4. **REC** (third press): kept and saved at once (no SAVE needed). If anything else was playing it plays on (SAVED,
   PLAYING); if not, it waits (SAVED - PLAY: HEAR IT) and PLAY brings it in. If nothing was played the pool stays
   empty (NOTHING RECORDED).

GLO during a take cancels it (TAKE CANCELLED) and returns to playback. A take can't be added to after it's kept:
record another loop instead, or delete it and record it again. All sixteen pools full says ALL 16 LOOPS USED,
EDIT + KEY: DELETE.

Tips:

- Record a drum loop first with the Drum kit (it's instrument 20, just after the original 19), click on, snap
  1/16, then layer other loops over it with PLAY running.
- For held chords that should fill each bar exactly, use snap BAR and hold the chord for most of the bar.
- The loop lengths (1, 2, 4 bars) all line up, and so do the preset banks' loops, so recordings stay in time with
  each other.
- Autoplay (SEQ) works on recorded loops too.

### Deleting a loop

Hold **EDIT**: the screen says DELETE A LOOP, TAP ITS KEY, and the keys of recorded loops light up. Still holding
EDIT, tap the loop's white key (its pool blinks red, TAP AGAIN TO DELETE), then tap the same key again: DELETED,
saved at once. Any other key cancels (DELETE CANCELLED). Works in either mode, but not during a take.

## What's kept

Each recorded loop is saved the moment it's kept or deleted, with its instrument, length and snap, and survives
power-off and Tidepool updates. SAVE additionally keeps the recorder's settings (instrument, length, snap, click,
octave) and any loop settings changed with the knobs. Each loop holds up to 256 notes.
