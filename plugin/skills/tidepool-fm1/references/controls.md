# Controls (playback)

How Tidepool is played on any bank. The Blank Canvas's recording mode changes some of these: see `canvas.md`.

## The screen

From the top:

- **The banner:** the bank's name over an animated pixel-art scene, and the tempo (BPM) on the right.
- **The chords:** the bank's four chords, the one playing now lit. While autoplay or a chord change is waiting, the
  new chords take over at the next bar line.
- **The pools:** sixteen rounded cells in four rows of four, one per loop, named. A pool glows in its row's colour
  while its loop plays, brighter as it gets louder. The row the knobs control is marked, and each of its pools shows
  a bar with the current setting's value.
- **The footer:** the setting the knobs change (for example `< VOLUME >`), AUTOPLAY or ENDING while autoplay is on,
  and the FM-1's audio load as CPU %.
- **Messages:** pressing anything shows what it did for a moment in the banner area (for example TEMPO 80 BPM). See
  `troubleshooting.md` for what they mean.

## Loops

| Control | What it does |
|---|---|
| White keys 1-16 | Turn a loop on or off. Keys 1-4 are the top row of pools, 5-8 the second, 9-12 the third, 13-16 the bottom. A loop joins on the beat: its key and pool blink until it does, then its key stays lit. Turning one off fades it out |
| Black keys | Not used in playback (the screen says NOT USED YET). In the Blank Canvas's recording mode they play notes |

The rows are, top to bottom: drones and pads, melodies, beats, then textures (the scene's atmosphere: rain, wind,
fire, crowds). Textures start at 25 % volume, the others at 70 %. `banks.md` lists every bank's loops by key.

## Adjusting loops

| Control | What it does |
|---|---|
| OCT- / OCT+ | Choose what the four knobs adjust: VOLUME, ATTACK, DECAY, REVERB, ECHO, PAN (shown in the footer). On the Blank Canvas there's a seventh, INSTRUMENT, which changes a recorded loop's instrument |
| ALGORITHM | Choose which row the knobs adjust. Pressing a loop's key also selects its row |
| KNOB1-4 | Adjust the selected setting for the selected row's four loops, one knob per loop, left to right |

- **Volume:** how loud the loop is.
- **Attack:** how softly its notes start (higher is slower, more swelling).
- **Decay:** how long its notes ring.
- **Reverb:** how much space.
- **Echo:** how much delay.
- **Pan:** left to right.

Every bank keeps its own settings.

## Bank, tempo, volume, saving

| Control | What it does |
|---|---|
| PRESETS | Change bank. The bank changes once the knob stops turning (about 0.7 s): the loops fade out, then the new bank is ready with nothing playing. Twelve preset banks, then the Blank Canvas |
| SELECT | Tempo, 56-140 BPM. Loops, autoplay and chord changes all follow it |
| MASTER | Main volume |
| SAVE | Remember the tempo, the current bank, every loop's settings in every bank, and every bank's chord progression, so they come back after switching off. SAVE is lit whenever something is unsaved. Loops recorded on the Blank Canvas save themselves straight away and don't need SAVE |

Without SAVE, changes to settings, tempo and chords are lost at power-off.

## Play, chords, autoplay, surprise

| Control | What it does |
|---|---|
| PLAY/STOP | A quick press stops everything (STOP, AGAIN: SAME MIX). Pressing again brings back the same loops. It acts when released. With nothing chosen yet it says PICK LOOPS ON THE KEYS |
| Hold PLAY, turn KNOB1 | Change the bank's chord progression: 0 is the bank's own, 1-99 are new ones in the same key and mood, each the same every time. It changes at the next bar line. Holding PLAY for a moment shows KNOB 1: CHORDS and outlines the chords; letting go without turning does nothing (it doesn't stop the music) |
| SEQ | Autoplay (below) |
| Hold HOME for 3 seconds | Surprise me: a random mix of loops with random settings. A countdown shows; letting go early cancels. Not on the Blank Canvas |
| FX, SEL, ENV, LFO, ARP | Not used (NOT USED YET) |
| GLO, REC, EDIT | The Blank Canvas's recording controls (`canvas.md`). On a preset bank they say BLANK CANVAS ONLY |

### Autoplay (SEQ)

SEQ hands the bank over to Tidepool, so the player can just listen.

- **When it acts:** once every four bars, so more often at higher tempos.
- **Building up:** each cycle it brings in or takes out a loop. From nothing it adds one per cycle, every row
  first, until there are about two loops in each of the top three rows and one texture (seven loops). At 72 BPM
  that takes about a minute and a half.
- **Once full:** it mostly swaps a loop for another in the same row, now and then drops one for a cycle, and
  sometimes does nothing. It never leaves a row empty.
- **Textures:** only one plays at a time, and it changes rarely (after at least 64 bars).
- **Chords:** on preset banks it moves to a new progression every 16 bars or so.

SEQ lights up and the footer says AUTOPLAY. It works on every bank, including the Blank Canvas (with its recorded
loops; with none it says NOTHING RECORDED YET). Changing bank keeps it going.

**To end it:**

- **SEQ again:** ends it gracefully. Every four bars, fewer loops: two at a time while more than four play, then
  one at a time; the beats first, the drones last. SEQ blinks and the footer says ENDING. When the last fades it
  says AUTOPLAY THE END. Pressing SEQ during the ending changes your mind and autoplay carries on.
- **PLAY:** stops everything at once and turns autoplay off.
- **GLO** (recording mode on the Blank Canvas) also turns it off.

## Sound out

The FM-1's headphone/line output, and over USB: connected to a computer, the FM-1 also appears as a USB audio
device, so Tidepool can be recorded straight into a computer. Tidepool doesn't send or receive MIDI notes.
