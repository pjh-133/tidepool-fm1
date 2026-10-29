/* Tidepool on the FM-1: new chord progressions for a bank, in its own key and style.
 *
 * tp_harmony(bank, n, ...) is progression n of the bank: 0 is the bank's own; 1, 2, ... are generated, each the same
 * every time. A progression keeps the bank's key and mode (read from its first chord: Shallows D minor, Forest Floor G
 * major, ...), starts on the home chord, and moves by the usual pulls of functional harmony (the last chord leaning
 * home). Its chords take the bank's flavour (TP_STYLE_*: 7ths and 9ths, add9 and sus4, or plain triads), voiced to
 * move as little as possible from one chord to the next, in the bank's register. The note grid (Kalimba, Marimba,
 * Harpsichord) keeps its rhythm, each note moved to the nearest scale note that is in the chord under it.
 * The loops that read the chord (pads, arps, bass lines, choirs) follow on their own. */
#pragma once
#include "tp_engine.h"

#define TP_HARMONY_MAX 99                      /* progressions 0 .. 99 */

void tp_harmony(const tp_bank_t *bank, int n, tp_chord_t chords[4], uint8_t kal[64]);
