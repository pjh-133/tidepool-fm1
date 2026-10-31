/* Tidepool on the FM-1: Bank 13, Cyberpunk. Written for the FM-1 (no web app original): a megacity in the rain, in
 * the mood of Blade Runner, in E minor, slow. Brass that swells and wavers, as an old polysynth's does under the
 * player's hand, a low drone, a glass pad and a dark choir; acid rain, a flying car passing over, a street announcer
 * far off and sirens further; a lonely sax, a piano remembered, bells and the bleeps of an interview machine; a big
 * drum in a big room, metal struck, a sequenced bass and ticking. Everything wet with reverb. */
#include "tp_voice.h"
#include "tp_math.h"

#define SR TP_SR
#define off_after(off, sec) ((off) + (int)((sec) * SR))

static const tp_chord_t CHORDS[4] = {
    { "Em9", 40, { 50, 54, 55, 59, 64 } },
    { "Cmaj7", 36, { 52, 55, 59, 60, 64 } },
    { "Am9", 45, { 52, 55, 57, 59, 60 } },
    { "B7sus4", 35, { 52, 54, 57, 59, 64 } },
};
static const uint8_t P13[8] = { 64, 67, 69, 71, 74, 76, 79, 81 };   /* E minor pentatonic */

/* ---- Brass ---- */
static void brass_swell(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float len = 32.0f * tp_sps();
    tp_voice_t *v;
    int i;
    if (s % 32)
        return;
    v = tv_new(l, off);                       /* four detuned saws, the filter opening slowly, then closing */
    for (i = 0; i < 4; i++)
        tv_osc(v, W_SAW, tp_mtof(c->notes[i + 1]), i % 2 ? 9.0f : -9.0f, 1.0f);
    tv_filter(v, F_LP, 350.0f, 2.0f);
    tv_filter_ramp(v, 1, 2600.0f, 1.6f);
    tv_filter_ramp(v, 1, 900.0f, 3.5f);
    tv_filter_ramp(v, 1, 600.0f, len > 5.6f ? len - 5.1f : 0.5f);
    tv_lfo(v, LFO_DET, 4.6f, 6.0f, 15);       /* the hand on the keys: a slow, wide vibrato */
    tv_env_asr(v, 0.7f, 0.04f, len - 0.7f, 2.5f);
    tv_len(v, len + 2.7f);
}

static void low_drone(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float bar = 16.0f * tp_sps(), f;
    tp_voice_t *v;
    int m;
    if (s % 16)
        return;
    m = c->root;
    while (m > 40)
        m -= 12;
    f = tp_mtof(m);
    v = tv_new(l, off);
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_osc(v, W_SAW, f, 5.0f, 0.3f);
    tv_osc(v, W_SINE, f * 1.5f, 0.0f, 0.25f);
    tv_filter(v, F_LP, 260.0f, 3.0f);
    tv_lfo(v, LFO_FC, 0.15f, 80.0f, 0);
    tv_env_asr(v, 1.0f, 0.2f, bar - 1.0f, 1.2f);
    tv_len(v, bar + 1.4f);
}

static void glass_pad(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float len = 32.0f * tp_sps();
    tp_voice_t *v;
    int i;
    if (s % 32 != 16)
        return;
    v = tv_new(l, off);                       /* high and cold, shimmering */
    for (i = 0; i < 4; i++)
        tv_osc(v, W_TRI, tp_mtof(c->notes[i] + 24), i % 2 ? 4.0f : -4.0f, 1.0f);
    tv_filter(v, F_HP, 600.0f, 0.0f);
    tv_lfo(v, LFO_AMP, 0.3f, 0.25f, 0);
    tv_env_asr(v, 3.0f, 0.03f, len - 3.0f, 3.0f);
    tv_len(v, len + 3.2f);
}

static void dark_choir(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float bar = 16.0f * tp_sps();
    tp_voice_t *v;
    int i;
    if (s % 16)
        return;
    v = tv_new(l, off);                       /* low voices on "ah": saws through two vowel bands */
    for (i = 0; i < 3; i++)
        tv_osc(v, W_SAW, tp_mtof(c->notes[i] - 12), (float)(i * 6 - 6), 1.0f);
    tv_filter(v, F_BP, 650.0f, 4.0f);
    tv_filter(v, F_BP, 1100.0f, 5.0f);
    tv_filter_parallel(v);
    tv_lfo(v, LFO_FC, 0.2f, 60.0f, 0);
    tv_lfo(v, LFO_DET, 5.0f, 5.0f, 7);
    tv_env_asr(v, 1.5f, 0.17f, bar - 1.5f, 1.5f);
    tv_len(v, bar + 1.7f);
}

/* ---- Streets ---- */
static void acid_rain_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);           /* heavy rain on concrete and canopies */
    tv_persist(v);
    tv_noise(v, 1, 1.0f);
    tv_filter(v, F_BP, 1400.0f, 0.6f);
    tv_env_const(v, 0.35f);
    tv_lfo(v, LFO_GAIN, 0.07f, 0.1f, 0);
}
static void acid_rain(int l, int s, int off)
{
    if (tp_rand() < 0.6f)                     /* drips off a canopy, a drop on a puddle */
        tp_burst(l, off_after(off, tp_rand() * tp_sps()), F_BP, 1800.0f + tp_rand() * 3500.0f, 6.0f, 0.001f,
                 0.03f + tp_rand() * 0.05f, 0.02f, 0.05f);
}

static void spinner(int l, int s, int off)
{
    tp_voice_t *v;
    if (s % 64 != 24 || tp_rand() > 0.6f)
        return;
    v = tv_new(l, off);                       /* a flying car passing over: the rush of air rising and falling */
    tv_noise(v, 0, 1.0f);
    tv_filter(v, F_BP, 300.0f, 1.2f);
    tv_filter_ramp(v, 1, 2400.0f, 2.2f);
    tv_filter_ramp(v, 1, 350.0f, 2.8f);
    tv_env_asr(v, 2.0f, 0.12f, 0.6f, 2.6f);
    tv_len(v, 5.4f);
    v = tv_new(l, off);                       /* and its engine's whine, dropping as it goes by */
    tv_osc(v, W_SINE, 880.0f, 0.0f, 1.0f);
    tv_osc(v, W_TRI, 1320.0f, 0.0f, 0.3f);
    tv_pitch_ramp(v, 620.0f, 4.5f);
    tv_env_asr(v, 2.0f, 0.02f, 0.6f, 2.6f);
    tv_len(v, 5.4f);
}

static void street_pa(int l, int s, int off)
{
    float len, f;
    tp_voice_t *v;
    if (s % 32 != 8 || tp_rand() > 0.45f)
        return;
    len = 1.2f + tp_rand() * 1.5f;
    f = 120.0f + tp_rand() * 40.0f;
    v = tv_new(l, off);                       /* an announcement, far off and echoing: a voice, no words */
    tv_osc(v, W_SAW, f, 0.0f, 1.0f);
    tv_pitch_ramp(v, f * 0.9f, len);
    tv_filter(v, F_BP, 700.0f, 5.0f);
    tv_filter(v, F_BP, 1600.0f, 6.0f);
    tv_filter_parallel(v);
    tv_lfo(v, LFO_FC, 3.1f + tp_rand(), 260.0f, 0);   /* the vowels moving: syllables */
    tv_lfo(v, LFO_AMP, 5.3f, 0.45f, 0);
    tv_env_asr(v, 0.05f, 0.16f, len, 0.2f);
    tv_len(v, len + 0.3f);
}

static void far_sirens(int l, int s, int off)
{
    tp_voice_t *v;
    if (s % 64 != 40 || tp_rand() > 0.4f)
        return;
    v = tv_new(l, off);                       /* rising and falling, blocks away */
    tv_osc(v, W_TRI, 700.0f, 0.0f, 1.0f);
    tv_lfo(v, LFO_DET, 0.35f, 280.0f, 1);
    tv_filter(v, F_LP, 1500.0f, 0.0f);
    tv_env_asr(v, 2.5f, 0.035f, 3.0f, 3.0f);
    tv_len(v, 8.6f);
}

/* ---- Memories ---- */
static void lonely_sax(int l, int s, int off)
{
    float len, f;
    tp_voice_t *v;
    if (s % 4 != 2 || tp_rand() > 0.2f || s % 64 >= 52)   /* a phrase, and a long breath */
        return;
    f = tp_mtof(P13[(int)(tp_rand() * 7.0f)] - 12);
    len = tp_sps() * (tp_rand() < 0.5f ? 4.0f : 8.0f);
    v = tv_new(l, off);                       /* breathy, scooping up into the note, a slow vibrato as it holds */
    tv_osc(v, W_SAW, f * 0.96f, 0.0f, 1.0f);
    tv_osc(v, W_SQR, f * 0.96f, 0.0f, 0.3f);
    tv_pitch_ramp(v, f, 0.15f);
    tv_lfo(v, LFO_DET, 4.5f, 10.0f, 3);
    tv_noise(v, 0, 0.08f);
    tv_filter(v, F_LP, 1400.0f, 1.5f);
    tv_env_asr(v, 0.15f, 0.045f, len, 0.8f);
    tv_len(v, len + 1.0f);
}

static void tears_piano(int l, int s, int off)
{
    int m = tp_kal[s % 64];
    float f;
    tp_voice_t *v;
    if (!m || tp_rand() > 0.5f)               /* a few notes, remembered, ringing a long time */
        return;
    f = tp_mtof(m);
    v = tv_new(l, off);
    tv_osc(v, W_TRI, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 2.0f, 0.0f, 0.4f);
    tv_osc(v, W_SINE, f * 3.01f, 0.0f, 0.12f);
    tv_filter(v, F_LP, 2400.0f, 0.0f);
    tv_env_ad(v, 0.002f, 0.07f, 2.8f);
    tv_part_decay(v, 2, 0.1f, 0.3f, 0.07f, 2.8f);
    tv_len(v, 3.0f);
}

static void bells(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float f;
    tp_voice_t *v;
    if (s % 16 != 8 || tp_rand() > 0.5f)
        return;
    f = tp_mtof(c->notes[1 + (int)(tp_rand() * 4.0f)] + 24);
    v = tv_new(l, off);                       /* a struck bell, its partials out of tune with each other */
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_fm(v, 3.5f, f * 1.2f, 1.5f);
    tv_osc(v, W_SINE, f * 2.76f, 0.0f, 0.2f);
    tv_env_ad(v, 0.001f, 0.07f, 3.5f);
    tv_part_decay(v, 1, 0.03f, 0.8f, 0.07f, 3.5f);
    tv_len(v, 3.7f);
}

static void bleeps(int l, int s, int off)
{
    int p = s % 32;
    float f;
    tp_voice_t *v;
    if (p >= 8 || tp_rand() > 0.6f)           /* a readout, a burst at a time */
        return;
    if (s % 64 >= 32 && tp_rand() > 0.4f)
        return;
    f = tp_mtof(P13[(int)(tp_rand() * 8.0f)] + 12);
    v = tv_new(l, off);
    tv_osc(v, W_SQR, f, 0.0f, 1.0f);
    tv_filter(v, F_LP, 3000.0f, 0.0f);
    tv_env_ad(v, 0.001f, 0.07f, 0.08f);
    tv_len(v, 0.1f);
}

/* ---- Pulse ---- */
static void big_drum(int l, int s, int off)
{
    int p = s % 16;
    tp_voice_t *v;
    if (p != 0 && !(p == 10 && tp_rand() < 0.4f))
        return;
    if (p == 0 && s % 32 && tp_rand() < 0.3f)
        return;
    v = tv_new(l, off);                       /* a great drum in a hall of concrete */
    tv_osc(v, W_SINE, 72.0f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, 118.0f, 0.0f, 0.3f);
    tv_pitch_ramp(v, 40.0f, 0.35f);
    tv_env_ad(v, 0.004f, p ? 0.45f : 0.7f, 1.1f);
    tv_len(v, 1.3f);
    tp_burst(l, off, F_LP, 400.0f, 0.0f, 0.002f, p ? 0.15f : 0.25f, 0.25f, 0.3f);
}

static void metal_hit(int l, int s, int off)
{
    int p = s % 32;
    float f;
    tp_voice_t *v;
    if ((p != 12 && p != 28) || tp_rand() > 0.55f)
        return;
    f = 180.0f + tp_rand() * 60.0f;
    v = tv_new(l, off);                       /* a girder struck, ringing */
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_fm(v, 1.41f, f * 5.0f, 0.4f);
    tv_osc(v, W_SINE, f * 2.73f, 0.0f, 0.4f);
    tv_osc(v, W_SINE, f * 4.07f, 0.0f, 0.2f);
    tv_filter(v, F_HP, 250.0f, 0.0f);
    tv_env_ad(v, 0.001f, 0.08f, 1.4f);
    tv_len(v, 1.6f);
}

static void seq_bass(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    static const int8_t pat[8] = { 0, 0, 12, 0, 7, 0, 12, 10 };
    int m;
    float f;
    tp_voice_t *v;
    if (s % 2)
        return;
    m = c->root;
    while (m > 40)
        m -= 12;
    m += pat[(s / 2) % 8];
    f = tp_mtof(m);
    v = tv_new(l, off);                       /* a sequencer's eighths, the filter breathing over the bars */
    tv_osc(v, W_SAW, f, 0.0f, 1.0f);
    tv_osc(v, W_SQR, f * 0.5f, 0.0f, 0.4f);
    tv_filter(v, F_LP, 500.0f + 400.0f * (float)(s % 64) / 64.0f, 6.0f);
    tv_filter_ramp(v, 1, 180.0f, 0.18f);
    tv_env_ad(v, 0.003f, 0.16f, 0.22f);
    tv_len(v, 0.3f);
}

static void ticks(int l, int s, int off)
{
    int p = s % 4;
    float pk = p == 0 ? 0.1f : p == 2 ? 0.07f : 0.04f;
    tp_burst(l, off, F_HP, 9000.0f, 0.7f, 0.0005f, pk * (0.8f + tp_rand() * 0.4f), 0.015f, 0.03f);
}

static const tp_loop_t LOOPS[TP_NLOOP] = {
    /* name             fam  pan     rev    dly    fade  start              sched */
    { "Brass swell",    0,  0.0f,  0.7f,  0.15f, 1.0f,  0,                 brass_swell },
    { "Low drone",      0,  0.0f,  0.3f,  0.0f,  1.0f,  0,                 low_drone },
    { "Glass pad",      0,  0.2f,  0.7f,  0.2f,  1.0f,  0,                 glass_pad },
    { "Dark choir",     0, -0.2f,  0.7f,  0.0f,  1.0f,  0,                 dark_choir },
    { "Acid rain",      1, -0.1f,  0.3f,  0.0f,  3.0f,  acid_rain_start,   acid_rain },
    { "Flying car",     1,  0.3f,  0.6f,  0.2f,  0.5f,  0,                 spinner },
    { "Street voice",   1, -0.3f,  0.8f,  0.4f,  0.5f,  0,                 street_pa },
    { "Far sirens",     1,  0.35f, 0.8f,  0.2f,  0.5f,  0,                 far_sirens },
    { "Lonely sax",     2,  0.15f, 0.7f,  0.3f,  0.5f,  0,                 lonely_sax },
    { "Tears piano",    2, -0.15f, 0.75f, 0.3f,  0.3f,  0,                 tears_piano },
    { "Bells",          2,  0.3f,  0.8f,  0.3f,  0.3f,  0,                 bells },
    { "Bleeps",         2, -0.3f,  0.4f,  0.4f,  0.3f,  0,                 bleeps },
    { "Big drum",       3,  0.0f,  0.8f,  0.0f,  0.05f, 0,                 big_drum },
    { "Metal hit",      3,  0.2f,  0.8f,  0.2f,  0.05f, 0,                 metal_hit },
    { "Seq bass",       3,  0.0f,  0.2f,  0.1f,  0.05f, 0,                 seq_bass },
    { "Ticks",          3, -0.25f, 0.3f,  0.2f,  0.05f, 0,                 ticks },
};

const tp_bank_t TP_CYBERPUNK = { "Cyberpunk", LOOPS, CHORDS, P13, 8, 113, 0.2f,
                                 { "Brass", "Streets", "Memories", "Pulse" }, TP_STYLE_LUSH, 0 };
