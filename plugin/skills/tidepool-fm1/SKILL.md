---
name: tidepool-fm1
description: Helps people play, record with and troubleshoot Tidepool, a custom ambient-loop firmware for the M-VAVE FM-1 synthesizer (thirteen synthesised scenes of sixteen loops, chord progressions, autoplay, and a Blank Canvas for recording loops on 36 instruments including drum kits). Use whenever someone asks how to do something in Tidepool or on an FM-1 running it, what a button, knob, key or screen message means, which bank or loop or instrument has a particular sound, how to record, save, delete or autoplay, how to install it or go back to the stock firmware, or why something isn't working as expected.
---

# Tidepool for the M-VAVE FM-1

Tidepool is an unofficial firmware for the M-VAVE FM-1, a pocket synth with 27 velocity-sensing keys (16 white,
11 black), 14 buttons, 7 knobs, a MASTER volume pot and a small square screen. It turns the FM-1 into an ambient
loop instrument. Each bank is a scene of sixteen loops (drones, melodies, beats and textures) that always fit
together in time and in key. Every sound is synthesised live, with no samples. A fourteenth bank, the Blank Canvas,
holds loops the player records.

Website: <https://pjh-133.github.io/tidepool-fm1/>, with the [web installer](https://pjh-133.github.io/tidepool-fm1/install/),
[a version that runs in the browser](https://pjh-133.github.io/tidepool-fm1/emu/) and
[the manual](https://pjh-133.github.io/tidepool-fm1/manual/). Source: <https://github.com/pjh-133/tidepool-fm1>.

## How to answer

- The person usually has the FM-1 in their hands. Give the steps in order, naming the physical control exactly as
  it is printed on the FM-1 (PLAY/STOP, PRESETS, OCT-, KNOB1, GLO, REC, EDIT, SEQ, HOME, SAVE), and say what they
  will see on the screen or lights when it worked.
- Keep it short and practical. One question usually needs one to five steps, not the whole manual.
- Read the reference file that covers the question before answering anything specific: the exact loop names, key
  numbers, instrument order and messages are there, and guessing them gets them wrong.
- If they quote something the screen said, look it up in `references/troubleshooting.md`: most messages are the
  firmware explaining exactly what it wants.
- If something isn't possible in Tidepool, say so plainly, and offer the nearest thing that is. Things it doesn't
  do: MIDI out of the notes, Bluetooth, sampling, editing a recorded loop's notes after it's kept (record a new one
  instead), more than sixteen recorded loops at once, the FX, SEL, ENV, LFO and ARP buttons (unused).
- Don't invent features, numbers or shortcuts. If the references don't cover it, say you're not sure and point to
  the manual or the GitHub repository's issues.

## Which reference to read

| Question is about | Read |
|---|---|
| What a control does, the screen layout, changing banks or tempo, the knobs' settings, saving, chords (PLAY + KNOB1), autoplay (SEQ), surprise (HOME) | `references/controls.md` |
| Which bank or loop has a sound, what each key plays in a bank, a bank's chords or mood | `references/banks.md` |
| Recording, the count-in, snap, layering, deleting, the Blank Canvas's playback mode | `references/canvas.md` |
| The 36 instruments, which key plays which drum | `references/instruments.md` |
| A screen message, something not working, installing, updating, going back to the stock firmware | `references/troubleshooting.md` |

## The essentials, for quick answers

- **Turn loops on and off:** white keys 1-16, four rows of four, matching the pools on the screen. A loop waits
  for the beat to join, blinking until it does.
- **Adjust loops:** OCT- / OCT+ pick what the knobs change (Volume, Attack, Decay, Reverb, Echo, Pan); ALGORITHM
  picks the row; KNOB1-4 change that row's four loops.
- **Banks:** turn PRESETS. **Tempo:** turn SELECT (56-140 bpm). **Volume:** MASTER.
- **Start / stop:** after switching on or changing bank nothing plays: pick loops on the keys (they blink), then
  PLAY starts them together from bar 1. While playing, PLAY stops; PLAY again brings back the same mix.
- **Chords:** hold PLAY and turn KNOB1 (0 is the bank's own progression, 1-99 are others in the same key).
- **Autoplay:** SEQ. SEQ again ends it gracefully, fading loops out over a few bars.
- **Random mix:** hold HOME for 3 seconds.
- **Keep settings:** SAVE (lit when something's unsaved). Recorded loops save themselves.
- **Record:** go to the Blank Canvas (the last bank), press GLO, choose an instrument with PRESETS, then press REC
  three times: set up, count in, keep.
