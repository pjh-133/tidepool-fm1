/* Tidepool on the FM-1: the Blank Canvas. Recorded loops (tp_user), its 36 instruments (tp_instruments.c), the
 * keys played live, and the recorder (tp_engine.c).
 *
 * Recording, as the user designed it: the app arms a slot (its instrument, length and snap set in tp_user first);
 * at the next bar line a one-bar count-in clicks, then the take starts on the bar after, in time with the shared
 * 64-step cycle. It loops round the length, each pass adding what is played to what is there (and playing it back),
 * until the app finishes it. The keys reach the engine as events (tp_live_on / tp_live_off), timed by the engine
 * against the audio itself. Threads: the app calls these from the main loop; the engine acts on them at the start of
 * its next block. */
#pragma once
#include "tp_engine.h"
#include "tp_voice.h"

#define TP_NINSTR 36                             /* the web app's 19, then the FM-1's 17 */
#define TP_MAXNOTES 256                          /* a loop's notes: 6 bytes each */

typedef struct {
    uint16_t pos, dur;                           /* in 1/256 of a step, pos from the loop's start */
    uint8_t note, vel;                           /* MIDI note, 1..127 */
} tp_note_t;

typedef struct {
    uint8_t used, instr, bars, snap;             /* bars 1, 2, 4; snap TP_SNAP_* */
    uint16_t n;
    tp_note_t notes[TP_MAXNOTES];
} tp_user_t;
extern tp_user_t tp_user[TP_NLOOP];

/* snap, applied as a loop plays (what was played is kept): off, to sixteenths, to eighths, or to the bar: a note (a
 * held chord) starts on the bar line (up to a beat early counts as on it) and lasts whole bars, at least one */
enum { TP_SNAP_OFF, TP_SNAP_16, TP_SNAP_8, TP_SNAP_BAR, TP_NSNAP };
/* where a loop's note plays and for how long, snapped, in 1/256 of a step (pos within the loop) */
void tp_user_place(const tp_user_t *U, const tp_note_t *N, int32_t *pos, int32_t *dur);

/* the instruments: their names (as the web app's), and a note: a voice on loop's bus, off samples into the block,
 * vel 0..1, dur seconds (< 0: held until tp_live_off) */
const char *tp_instr_name(int instr);
int tp_instr_drums(int instr);                   /* a drum kit: a sound a key (F3 = 53 .. G5 = 79), no octave */
tp_voice_t *tp_instr_note(int loop, int off, int instr, int midi, float vel, float dur);

/* the keys, live (bus TP_LIVE): note on with velocity 1..127 (an id for its note off; 0 if the queue is full) */
uint16_t tp_live_on(int instr, int midi, int vel);
void tp_live_off(uint16_t id);
void tp_live_all_off(void);

/* the recorder */
enum { TP_REC_OFF, TP_REC_COUNTIN, TP_REC_ON };
void tp_rec_arm(int slot, int click);           /* tp_user[slot]'s instrument, bars and snap set first */
void tp_rec_finish(int keep_playing);           /* the take kept; the slot plays on, or stops */
void tp_rec_cancel(void);                       /* the take gone, the slot empty again */
typedef struct {
    int state, slot, pass;                       /* TP_REC_*, the slot, the pass (0 the first) */
    int beats_to_go;                             /* count-in: beats before the take (4 .. 1), more while waiting */
    float pos;                                   /* recording: where in the loop, in steps (as its notes are placed) */
} tp_rec_info_t;
void tp_rec_info(tp_rec_info_t *info);

/* the Blank Canvas bank (bank_canvas.c): its loops play tp_user */
extern const tp_bank_t TP_CANVAS;
