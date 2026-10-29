/* Tidepool on the FM-1: Bank 7, Dunes. Written for the FM-1 (no web app original): a desert night by a campfire,
 * in C minor. A tanpura-like drone, a warm pad and low strings; sand wind, the fire, camel bells and crickets; oud,
 * ney flute, santur and a distant voice; darbuka, frame drum, finger cymbals and riq jingles. */
#include "tp_voice.h"
#include "tp_math.h"

#define SR TP_SR
#define off_after(off, sec) ((off) + (int)((sec) * SR))

static const tp_chord_t CHORDS[4] = {
    { "Cm", 36, { 48, 51, 55, 60, 63 } },
    { "Fm", 41, { 48, 53, 56, 60, 65 } },
    { "Bbsus4", 34, { 51, 53, 58, 63, 65 } },
    { "Ab", 44, { 48, 51, 56, 60, 63 } },
};
static const uint8_t P7[8] = { 60, 63, 65, 67, 68, 70, 72, 75 };   /* C minor, its sixth and seventh kept */

/* ---- Sands ---- */
static void tanpura(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);           /* C and G, buzzing saws behind a slow filter, a low C under them */
    tv_persist(v);
    tv_osc(v, W_SINE, tp_mtof(24), 0.0f, 1.0f);
    tv_osc(v, W_SAW, tp_mtof(36), -3.0f, 0.3f);
    tv_osc(v, W_SAW, tp_mtof(43), 4.0f, 0.25f);
    tv_osc(v, W_SAW, tp_mtof(48), 0.0f, 0.15f);
    tv_filter(v, F_LP, 650.0f, 4.0f);
    tv_env_const(v, 0.15f);
    tv_lfo(v, LFO_FC, 0.11f, 380.0f, 0);
}

static void night_pad(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float bar = 16.0f * tp_sps();
    int h, i;
    if (s % 16)
        return;
    for (h = 0; h < 2; h++) {
        tp_voice_t *v = tv_new(l, off);
        for (i = h * 2; i < h * 2 + 2; i++) {
            tv_osc(v, W_TRI, tp_mtof(c->notes[i + 1]), i % 2 ? -5.0f : 5.0f, 1.0f);
            tv_osc(v, W_SAW, tp_mtof(c->notes[i + 1]), 0.0f, 0.25f);
        }
        tv_filter(v, F_LP, 1100.0f, 0.7f);
        tv_env_asr(v, 1.2f, 0.05f, bar - 1.2f, 1.6f);
        tv_len(v, bar + 1.8f);
    }
}

static void low_strings(int l, int s, int off)
{
    float len = 32.0f * tp_sps(), f;
    tp_voice_t *v;
    if (s % 32)
        return;
    f = tp_mtof(tp_chord_at(s)->root + 12);
    v = tv_new(l, off);
    tv_osc(v, W_SAW, f, -7.0f, 1.0f);
    tv_osc(v, W_SAW, f, 7.0f, 1.0f);
    tv_osc(v, W_SAW, f * 1.5f, 0.0f, 0.4f);
    tv_lfo(v, LFO_DET, 4.8f, 7.0f, 7);
    tv_filter(v, F_LP, 650.0f, 1.0f);
    tv_env_asr(v, 1.2f, 0.06f, len - 1.2f, 1.8f);
    tv_len(v, len + 2.0f);
}

static void dune_swell(int l, int s, int off)
{
    int p = s % 16;
    float f;
    tp_voice_t *v;
    if (p != 0 && p != 10)
        return;
    f = tp_mtof(tp_chord_at(s)->root + (p ? 7 : 0));
    v = tv_new(l, off);
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_osc(v, W_TRI, f * 2.0f, 0.0f, 0.15f);
    tv_env_ad(v, 0.08f, p ? 0.22f : 0.32f, 1.6f);
    tv_len(v, 1.8f);
}

/* ---- Desert ---- */
static void sand_wind_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);
    tv_persist(v);
    tv_noise(v, 0, 1.0f);
    tv_filter(v, F_BP, 1200.0f, 2.0f);
    tv_env_const(v, 0.22f);
    tv_lfo(v, LFO_FC, 0.07f, 900.0f, 0);
    tv_lfo(v, LFO_GAIN, 0.05f, 0.09f, 0);
}

static void campfire_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);           /* the fire's low roar under its crackle */
    tv_persist(v);
    tv_noise(v, 1, 1.0f);
    tv_filter(v, F_LP, 380.0f, 0.7f);
    tv_env_const(v, 0.2f);
    tv_lfo(v, LFO_GAIN, 0.3f, 0.08f, 0);
}
static void campfire(int l, int s, int off)
{
    int n = (int)(tp_rand() * 3.0f), i;
    for (i = 0; i < n; i++) {
        int at = off_after(off, tp_rand() * tp_sps());
        tp_burst(l, at, F_BP, 1500.0f + tp_rand() * 3500.0f, 3.0f, 0.001f, 0.04f + tp_rand() * 0.1f,
                 0.01f + tp_rand() * 0.03f, 0.05f);
    }
}

static void camel_bells(int l, int s, int off)
{
    int i, n;
    if (s % 8 || tp_rand() > 0.3f)
        return;
    n = 2 + (int)(tp_rand() * 3.0f);         /* a few bells swinging, a step apart */
    for (i = 0; i < n; i++) {
        float f = 900.0f + tp_rand() * 500.0f;
        tp_voice_t *v = tv_new(l, off_after(off, i * tp_sps() * (0.8f + tp_rand() * 0.6f)));
        tv_osc(v, W_SINE, f, 0.0f, 1.0f);
        tv_fm(v, 2.76f, f * 0.9f, 0.2f);
        tv_env_ad(v, 0.002f, 0.09f - i * 0.012f, 0.9f);
        tv_len(v, 1.0f);
    }
}

static void desert_crickets(int l, int s, int off)
{
    float f;
    int t0, i;
    if (tp_rand() > 0.14f)
        return;
    f = 5200.0f + tp_rand() * 600.0f;
    t0 = off_after(off, tp_rand() * tp_sps());
    for (i = 0; i < 4; i++) {
        tp_voice_t *v = tv_new(l, off_after(t0, i * 0.04f));
        tv_osc(v, W_SINE, f, 0.0f, 1.0f);
        tv_env_ad(v, 0.003f, 0.022f, 0.025f);
        tv_len(v, 0.05f);
    }
}

/* ---- Caravan ---- */
static void oud(int l, int s, int off)
{
    int m = tp_kal[s % 64];
    float f;
    tp_voice_t *v;
    if (!m)
        return;
    f = tp_mtof(m - 12);
    v = tv_new(l, off);                       /* a plucked, fretless string: bright, closing fast */
    tv_osc(v, W_SAW, f, 0.0f, 1.0f);
    tv_osc(v, W_TRI, f, 6.0f, 0.6f);
    tv_filter(v, F_LP, 3200.0f, 3.0f);
    tv_filter_ramp(v, 1, 600.0f, 0.25f);
    tv_env_ad(v, 0.002f, 0.07f, 0.9f);
    tv_len(v, 1.0f);
}

static void ney_flute(int l, int s, int off)
{
    float len = tp_sps() * 6.0f, f;
    tp_voice_t *v;
    if (s % 16 != 4 || tp_rand() > 0.5f)
        return;
    f = tp_mtof(P7[2 + (int)(tp_rand() * 6.0f)]);
    v = tv_new(l, off);                       /* breathy, sliding up into the note */
    tv_osc(v, W_SINE, f * 0.94f, 0.0f, 1.0f);
    tv_osc(v, W_TRI, f * 0.94f, 0.0f, 0.15f);
    tv_pitch_ramp(v, f, 0.25f);
    tv_lfo(v, LFO_DET, 5.0f, 9.0f, 3);
    tv_env_asr(v, 0.25f, 0.08f, len, 0.9f);
    tv_len(v, len + 1.3f);
    tp_burst(l, off, F_BP, 2000.0f, 0.8f, 0.15f, 0.025f, 0.5f, 0.8f);
}

static void santur(int l, int s, int off)
{
    static const int8_t pat[16] = { 0, -1, 2, 3, -1, 2, 4, -1, 1, -1, 2, 3, 4, -1, 3, 2 };
    int idx = pat[s % 16];
    float f;
    tp_voice_t *v;
    if (idx < 0 || tp_rand() > 0.85f)
        return;
    f = tp_mtof(tp_chord_at(s)->notes[idx] + 12);
    v = tv_new(l, off);                       /* a hammered string pair, a little out from each other */
    tv_osc(v, W_TRI, f, -5.0f, 1.0f);
    tv_osc(v, W_TRI, f, 5.0f, 1.0f);
    tv_osc(v, W_SINE, f * 2.0f, 0.0f, 0.5f);
    tv_filter(v, F_LP, 3500.0f, 0.7f);
    tv_env_ad(v, 0.001f, 0.035f, 1.1f);
    tv_part_decay(v, 2, 0.02f, 0.2f, 0.035f, 1.1f);
    tv_len(v, 1.2f);
}

static void desert_voice(int l, int s, int off)
{
    float len = 16.0f * tp_sps(), f;
    tp_voice_t *v;
    if (s % 32 != 16 || tp_rand() > 0.7f)
        return;
    f = tp_mtof(tp_chord_at(s)->notes[2 + (int)(tp_rand() * 3.0f)]);
    v = tv_new(l, off);                       /* an "aah": a saw through two formants */
    tv_osc(v, W_SAW, f, -4.0f, 1.0f);
    tv_osc(v, W_SAW, f, 4.0f, 1.0f);
    tv_lfo(v, LFO_DET, 5.3f, 10.0f, 3);
    tv_filter(v, F_BP, 750.0f, 5.0f);
    tv_filter(v, F_BP, 1150.0f, 6.0f);
    tv_filter_parallel(v);
    tv_env_asr(v, 1.5f, 0.22f, len - 1.5f, 2.0f);
    tv_len(v, len + 2.2f);
}

/* ---- Rhythm ---- */
static void darbuka(int l, int s, int off)
{
    int p = s % 16;
    tp_voice_t *v;
    switch (p) {
    case 0: case 7:                           /* doum: the deep centre */
        v = tv_new(l, off);
        tv_osc(v, W_SINE, 120.0f, 0.0f, 1.0f);
        tv_pitch_ramp(v, 80.0f, 0.1f);
        tv_env_ad(v, 0.002f, p ? 0.5f : 0.7f, 0.3f);
        tv_len(v, 0.4f);
        return;
    case 4: case 10: case 12: case 14:        /* tek: the rim */
        tp_burst(l, off, F_BP, 3800.0f, 2.5f, 0.001f, p == 4 || p == 12 ? 0.3f : 0.17f, 0.04f, 0.08f);
        return;
    default:
        if (tp_rand() < 0.12f)                /* now and then a ghost note */
            tp_burst(l, off, F_BP, 3800.0f, 2.5f, 0.001f, 0.05f, 0.03f, 0.06f);
        return;
    }
}

static void frame_drum(int l, int s, int off)
{
    int p = s % 32;
    tp_voice_t *v;
    if (p != 0 && p != 20)
        return;
    v = tv_new(l, off);
    tv_osc(v, W_SINE, 70.0f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, 118.0f, 0.0f, 0.3f);
    tv_pitch_ramp(v, 52.0f, 0.3f);
    tv_env_ad(v, 0.004f, 0.55f, 0.7f);
    tv_len(v, 0.8f);
    tp_burst(l, off, F_LP, 900.0f, 0.7f, 0.002f, 0.12f, 0.08f, 0.15f);
}

static void finger_cymbals(int l, int s, int off)
{
    tp_voice_t *v;
    if (s % 16 != 8 || tp_rand() > 0.6f)
        return;
    v = tv_new(l, off);
    tv_osc(v, W_SINE, 2650.0f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, 4120.0f, 0.0f, 0.6f);
    tv_fm(v, 1.53f, 1800.0f, 0.15f);
    tv_env_ad(v, 0.001f, 0.05f, 1.4f);
    tv_len(v, 1.5f);
}

static void riq_jingles(int l, int s, int off)
{
    if (s % 2 == 0 && s % 4 != 0)
        tp_burst(l, off, F_HP, 7000.0f, 0.7f, 0.003f, 0.05f, 0.09f, 0.14f);
    else if (s % 4 == 0 && tp_rand() < 0.3f)
        tp_burst(l, off, F_HP, 7000.0f, 0.7f, 0.003f, 0.025f, 0.06f, 0.1f);
}

static const tp_loop_t LOOPS[TP_NLOOP] = {
    /* name             fam  pan     rev    dly    fade  start            sched */
    { "Tanpura",        0,  0.0f,  0.3f,  0.0f,  4.0f,  tanpura,         0 },
    { "Night pad",      0, -0.15f, 0.55f, 0.0f,  1.0f,  0,               night_pad },
    { "Low strings",    0,  0.15f, 0.5f,  0.0f,  1.0f,  0,               low_strings },
    { "Dune swell",     0,  0.0f,  0.25f, 0.0f,  0.5f,  0,               dune_swell },
    { "Sand wind",      1,  0.0f,  0.3f,  0.0f,  3.0f,  sand_wind_start, 0 },
    { "Campfire",       1, -0.2f,  0.2f,  0.0f,  2.0f,  campfire_start,  campfire },
    { "Camel bells",    1,  0.35f, 0.5f,  0.2f,  0.5f,  0,               camel_bells },
    { "Crickets",       1, -0.3f,  0.25f, 0.0f,  1.0f,  0,               desert_crickets },
    { "Oud",            2, -0.2f,  0.4f,  0.3f,  0.3f,  0,               oud },
    { "Ney flute",      2,  0.25f, 0.65f, 0.35f, 0.5f,  0,               ney_flute },
    { "Santur",         2,  0.1f,  0.4f,  0.25f, 0.3f,  0,               santur },
    { "Distant voice",  2, -0.1f,  0.75f, 0.3f,  0.5f,  0,               desert_voice },
    { "Darbuka",        3,  0.1f,  0.3f,  0.0f,  0.05f, 0,               darbuka },
    { "Frame drum",     3, -0.1f,  0.4f,  0.0f,  0.05f, 0,               frame_drum },
    { "Finger cymbals", 3,  0.3f,  0.5f,  0.3f,  0.05f, 0,               finger_cymbals },
    { "Riq jingles",    3, -0.3f,  0.3f,  0.0f,  0.05f, 0,               riq_jingles },
};

const tp_bank_t TP_DUNES = { "Dunes", LOOPS, CHORDS, P7, 8, 57, 0.24f, { "Sands", "Desert", "Caravan", "Rhythm" },
                             TP_STYLE_FOLK, 0 };
