# Troubleshooting, messages, installing

## "Why doesn't..."

| Problem | Why, and what to do |
|---|---|
| I pressed a key and the loop didn't start | Loops join on the beat: the key and pool blink until it comes in (slow loops like drones and pads also fade in over a few seconds). If nothing blinks, check the music isn't stopped (press PLAY), the MASTER volume, and that the loop's VOLUME isn't at 0 (OCT- / OCT+ to VOLUME, then that row's knob) |
| A texture is very quiet | Textures (the bottom row) start at 25 % on purpose. Choose VOLUME with OCT- / OCT+, select the bottom row with ALGORITHM or by pressing a texture's key, and turn its knob up |
| My settings were gone after switching off | Settings, tempo and chord progressions are only kept after SAVE (lit when something's unsaved). Recorded loops save themselves |
| Holding PLAY to change chords stopped the music | It doesn't on current versions: a hold long enough to show KNOB 1: CHORDS never stops. A quick tap does stop (and another quick tap brings the same mix back) |
| The chords didn't change straight away | A new progression starts at the next bar line, by design, so it lands in time |
| Autoplay isn't adding loops quickly | It acts once every four bars (13 seconds at 72 BPM); from nothing it takes about seven cycles to fill. A faster tempo (SELECT) speeds it up |
| Autoplay won't start on the Blank Canvas | It needs recorded loops (NOTHING RECORDED YET) |
| The black keys do nothing | They're only used in the Blank Canvas's recording mode |
| HOME does nothing | Hold it for 3 seconds (a countdown shows). It doesn't work on the Blank Canvas |
| GLO / REC / EDIT say BLANK CANVAS ONLY | They're the recorder's: turn PRESETS to the last bank, the Blank Canvas |
| My recording is out of time / notes are late | Try snap 1/16 or 1/8 (KNOB2 before recording, or for a kept loop record it again with snap on). Snap is applied on playback, so the timing played is kept but heard tidied |
| I want to fix one wrong note in a recorded loop | Kept loops can't be edited. Delete it (hold EDIT, tap its key twice) and record it again, or record the correction as a separate loop |
| The drum kit ignores OCT- / OCT+ | On purpose: each key is always the same drum. `instruments.md` lists which key is which |
| The sound crackles or drops out | Too much for the FM-1 at once: the CPU % in the footer shows how busy it is. Turn off a loop or two, especially pads and strings, which cost the most |
| I can't hear it on my computer | Over USB the FM-1 is a USB audio device: choose it as the input in the computer's sound settings or recording software |

## Messages on the screen

| Message | Meaning |
|---|---|
| (a loop's name) JOINS ON THE BEAT / FADING OUT | Turned on (waiting for the beat) or off |
| (a loop's name) EMPTY - GLO TO RECORD | An empty pool on the Blank Canvas: record into it in recording mode |
| STOP, AGAIN: SAME MIX | PLAY stopped everything; PLAY again brings the same loops back |
| PLAY, PICK LOOPS ON THE KEYS | PLAY with nothing chosen yet: turn some loops on with the white keys |
| KNOB 1: CHORDS, NOW ORIGINAL / NOW NO. n | Holding PLAY: turn KNOB1 to change the progression (original = the bank's own) |
| CHANGING BANK | A key pressed while the bank changes: wait a moment |
| (bank name) READY / AUTOPLAY | The new bank is ready (with autoplay carrying on) |
| SAVED, BANKS AND TEMPO | SAVE worked |
| SAVE FAILED | The FM-1 couldn't write its memory. Try SAVE again; if it keeps failing, reinstall Tidepool |
| SURPRISE IN 3 / 2 / 1 | Holding HOME: keep holding for a random mix |
| AUTOPLAY, SEQ AGAIN: END | Autoplay started |
| AUTOPLAY, ENDING: SEQ TO GO ON | Autoplay is winding down; SEQ resumes it |
| AUTOPLAY THE END | The ending finished: everything has stopped, autoplay is off |
| (button) NOT USED YET | That button or black key does nothing in Tidepool |
| REC, GLO: RECORDING MODE | REC pressed outside recording mode: press GLO first |
| RECORDING MODE, REC: A NEW LOOP | Recording mode is on (AUTOPLAY OFF if autoplay was running) |
| NEW LOOP n, REC AGAIN: COUNT-IN | Set up in pool n; REC again to start the count-in |
| COUNT-IN FROM THE NEXT BAR | Armed: one bar of count-in, then recording |
| RECORDING, REC: DONE GLO: CANCEL | Recording now: REC keeps it, GLO throws it away |
| LOOP n SAVED, PLAYING / SAVED - PLAY: HEAR IT | Kept; it plays on, or PLAY brings it in |
| NOTHING RECORDED | The take had no notes; the pool stays empty |
| CANCELLED / TAKE CANCELLED | The count-in or take was cancelled |
| ALL 16 LOOPS USED, EDIT + KEY: DELETE | Every pool is full: delete one first |
| AFTER THE TAKE, REC: DONE | Recorder settings can't change during a count-in or take |
| DELETE A LOOP, TAP ITS KEY / TAP AGAIN TO DELETE / DELETED / DELETE CANCELLED | Deleting with EDIT held |
| OCTAVE, DRUMS: NONE | OCT- / OCT+ with a drum kit chosen: drum kits have no octave |
| PLAYBACK, KEYS: THE LOOPS | Back in playback mode |

## Installing, updating, going back

- **Install or update:** the [web installer](https://pjh-133.github.io/tidepool-fm1/install/) in **Chrome or Edge**
  (Safari and Firefox don't support Web MIDI, which it needs). Connect the FM-1 directly to the computer by USB,
  switch it on, press Install, and allow MIDI when the browser asks. Don't unplug it while it writes (about a
  minute). Command line instead: `python3 base/bsp/tools/fm1_install.py tidepool-X.Y.Z.fwsc` from a checkout of
  the repository (needs `pip install mido python-rtmidi`).
- **Saved settings and recorded loops survive updates** of Tidepool. Installing a different firmware, or going back
  to M-VAVE's, can lose them.
- **An install was cut off:** the FM-1 waits in update mode. Leave it connected and run the install again; it
  finishes the job.
- **Going back to M-VAVE's own firmware:** "Back to the stock firmware" on the web installer page, or M-VAVE's own
  updater (M-UPGRADE, from m-vave.com, Downloads). If the FM-1 won't start at all, the installer page links to a
  recovery tool.
- **m-bootloader** (offered by the web installer after installing): an open bootloader that gives a recovery menu
  (hold PLAY while switching on) and an update mode (hold REC), even if a firmware won't start.
- **Which version am I running?** The web installer shows it after an install; release builds report an identity
  like FM-1_86010001 (version 1.0.1).
- **Try before installing:** the [browser version](https://pjh-133.github.io/tidepool-fm1/emu/) runs the same
  firmware with sound: press "Switch on", then the white keys (with the mouse, or the computer keyboard's shortcuts).

Tidepool is unofficial and not supported by M-VAVE. Problems and ideas: the GitHub repository's issues,
<https://github.com/pjh-133/tidepool-fm1/issues>.
