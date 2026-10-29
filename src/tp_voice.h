/* Tidepool on the FM-1: building a voice, for the banks.
 * tv_new() takes a slot (stealing the oldest note if the pool is full) and the calls after it set it
 * up, the way the web app wires nodes: tv_osc for each oscillator, tv_filter, then an envelope. Times
 * are in seconds, frequencies in Hz, offsets in samples from the start of the current block. */
#pragma once
#include "tp_engine.h"

#define TP_NPART 5

enum { W_SINE, W_TRI, W_SAW, W_SQR };
enum { F_LP = 1, F_HP, F_BP };
enum { LFO_FC, LFO_GAIN, LFO_DET, LFO_AMP };   /* cutoff (Hz), a constant sound's level, detune (cents), tremolo */

typedef struct tp_voice tp_voice_t;

tp_voice_t *tv_new(int loop, int off);          /* NULL when the pool is full of continuous sounds */
void tv_osc(tp_voice_t *v, int wave, float hz, float cents, float level);
void tv_part_decay(tp_voice_t *v, int part, float peak, float d, float env_peak, float env_d);
void tv_noise(tp_voice_t *v, int brown, float level);
void tv_filter(tp_voice_t *v, int type, float hz, float q);   /* q linear (BP), or Web Audio's dB (LP, HP) */
void tv_filter_ramp(tp_voice_t *v, int exp, float to_hz, float seconds);   /* filter 0's sweep, up to 3 stages */
void tv_filter_parallel(tp_voice_t *v);         /* the two filters side by side, summed (formants), not in series */
void tv_amp(tp_voice_t *v, float gain);         /* a gain after the envelope; LFO_AMP moves it (tremolo) */
void tv_env_ad(tp_voice_t *v, float a, float peak, float d);
void tv_env_asr(tp_voice_t *v, float a, float peak, float hold, float r);
void tv_env_const(tp_voice_t *v, float gain);
void tv_env_gate(tp_voice_t *v, float a, float peak, float r);   /* held until released (tp_live_off) */
void tv_set_id(tp_voice_t *v, uint16_t id);
void tv_len(tp_voice_t *v, float seconds);      /* stop(t + seconds) */
void tv_pitch_ramp(tp_voice_t *v, float to_hz, float t);       /* exponential, as Web Audio */
void tv_fm(tp_voice_t *v, float ratio, float index_hz, float decay);   /* modulates oscillator 0 */
void tv_lfo(tp_voice_t *v, int target, float rate, float depth, int part_mask);
void tv_persist(tp_voice_t *v);                 /* a continuous sound: lives until its loop stops */

void tp_burst(int loop, int off, int type, float f, float q, float a, float peak, float d, float len);
