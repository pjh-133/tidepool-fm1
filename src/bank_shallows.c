/* Tidepool on the FM-1: Bank 1, Shallows. A line-by-line port of BANK1_LOOPS in public/index.html.
 * Where the web app sends a chord's oscillators through one filter, each with its own (identical)
 * envelope, this makes the chord one voice with several oscillators: the same sound, far fewer voices. */
#include "tp_voice.h"
#include "tp_math.h"

#define SR TP_SR
#define off_after(off, sec) ((off) + (int)((sec) * SR))

static const tp_chord_t CHORDS[4] = {
    { "Dm9", 38, { 50, 53, 57, 60, 64 } },
    { "Bbmaj7", 34, { 46, 50, 53, 57, 62 } },
    { "Fmaj7", 41, { 53, 57, 60, 64, 67 } },
    { "C6/9", 36, { 48, 52, 55, 57, 62 } },
};
static const uint8_t P1[9] = { 62, 65, 67, 69, 72, 74, 77, 79, 81 };

/* ---- Drones ---- */
static void low_drone(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);
    tv_persist(v);
    tv_osc(v, W_SAW, tp_mtof(38), -7.0f, 0.5f);
    tv_osc(v, W_SAW, tp_mtof(38), 7.0f, 0.5f);
    tv_osc(v, W_SINE, tp_mtof(26), 0.0f, 0.9f);
    tv_osc(v, W_TRI, tp_mtof(45), 3.0f, 0.25f);
    tv_filter(v, F_LP, 420.0f, 2.0f);
    tv_env_const(v, 0.16f);
    tv_lfo(v, LFO_FC, 0.06f, 260.0f, 0);
}

static void air_pad(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float bar = 16.0f * tp_sps();
    tp_voice_t *v;
    int i;
    if (s % 16)
        return;
    v = tv_new(l, off);                /* the chord: five notes, one shared low-pass, one envelope */
    for (i = 0; i < 5; i++)
        tv_osc(v, W_TRI, tp_mtof(c->notes[i] + 12), i % 2 ? -6.0f : 6.0f, 1.0f);
    tv_filter(v, F_LP, 1400.0f, 0.7f);
    tv_env_asr(v, 1.4f, 0.045f, bar - 1.4f, 2.2f);
    tv_len(v, bar + 2.6f);
}

static void shimmer(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    int k = s / 8, i;
    tp_voice_t *v;
    if (s % 8)
        return;
    v = tv_new(l, off);                /* two notes, each with a copy 8 cents up: four sines, one envelope */
    for (i = 0; i < 2; i++) {
        float f = tp_mtof(c->notes[(k + 2 * i) % 5] + 24);
        tv_osc(v, W_SINE, f, 0.0f, 1.0f);
        tv_osc(v, W_SINE, f, 8.0f, 1.0f);
    }
    tv_env_asr(v, 0.9f, 0.022f, 0.4f, 3.0f);
    tv_len(v, 4.5f);
}

static void sub_pulse(int l, int s, int off)
{
    tp_voice_t *v;
    if (s % 8)
        return;
    v = tv_new(l, off);
    tv_osc(v, W_SINE, tp_mtof(tp_chord_at(s)->root), 0.0f, 1.0f);
    tv_env_ad(v, 0.05f, 0.42f, 1.7f);
    tv_len(v, 2.0f);
}

/* ---- Textures ---- */
static void rain_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);
    tv_persist(v);
    tv_noise(v, 0, 1.0f);
    tv_filter(v, F_HP, 900.0f, 0.7f);
    tv_filter(v, F_LP, 7000.0f, 0.7f);
    tv_env_const(v, 0.05f);
}
static void rain(int l, int s, int off)
{
    float sps = tp_sps();
    int n = tp_rand() < 0.6f ? 1 + (tp_rand() < 0.4f ? 1 : 0) : 0, i;
    for (i = 0; i < n; i++) {
        int at = off_after(off, tp_rand() * sps);
        float f = 1800.0f + tp_rand() * 4500.0f, pk = 0.04f + tp_rand() * 0.08f, d = 0.03f + tp_rand() * 0.04f;
        tp_burst(l, at, F_BP, f, 6.0f, 0.001f, pk, d, 0.1f);
    }
}

static void wind_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);
    tv_persist(v);
    tv_noise(v, 1, 1.0f);
    tv_filter(v, F_BP, 500.0f, 1.4f);
    tv_env_const(v, 0.45f);
    tv_lfo(v, LFO_FC, 0.05f, 300.0f, 0);
    tv_lfo(v, LFO_FC, 0.13f, 120.0f, 0);
    tv_lfo(v, LFO_GAIN, 0.07f, 0.25f, 0);
    v = tv_new(l, off);
    tv_persist(v);
    tv_noise(v, 0, 1.0f);
    tv_filter(v, F_BP, 1200.0f, 12.0f);
    tv_env_const(v, 0.07f);
    tv_lfo(v, LFO_FC, 0.09f, 400.0f, 0);
    tv_lfo(v, LFO_GAIN, 0.04f, 0.05f, 0);
}

static void vinyl_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);
    tv_persist(v);
    tv_noise(v, 0, 1.0f);
    tv_filter(v, F_HP, 4000.0f, 0.7f);
    tv_env_const(v, 0.012f);
}
static void vinyl(int l, int s, int off)
{
    float sps = tp_sps();
    int n = tp_rand() < 0.7f ? 1 + (int)(tp_rand() * 3.0f) : 0, i;
    for (i = 0; i < n; i++) {
        tp_voice_t *v = tv_new(l, off_after(off, tp_rand() * sps));
        tv_noise(v, 0, 1.0f);
        tv_filter(v, F_HP, 1200.0f, 0.7f);
        tv_env_const(v, 0.05f + tp_rand() * 0.25f);
        tv_len(v, 0.004f);
    }
}

static void ocean_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);
    tv_persist(v);
    tv_noise(v, 1, 1.0f);
    tv_filter(v, F_LP, 700.0f, 0.5f);
    tv_env_const(v, 0.35f);
    tv_lfo(v, LFO_GAIN, 0.09f, 0.3f, 0);
    tv_lfo(v, LFO_FC, 0.09f, 350.0f, 0);
}

/* ---- Melodies ---- */
static void glass_arp(int l, int s, int off)
{
    static const int8_t pat[8] = { 0, 2, 4, 1, 3, 2, 4, -1 };
    int idx;
    float f;
    tp_voice_t *v;
    if (s % 2)
        return;
    idx = pat[(s / 2) % 8];
    if (idx < 0)
        return;
    f = tp_mtof(tp_chord_at(s)->notes[idx % 5] + 12);
    v = tv_new(l, off);
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_fm(v, 3.5f, f * 2.0f, 0.45f);
    tv_env_ad(v, 0.002f, 0.06f, 1.4f);
    tv_len(v, 1.6f);
}

static void kalimba(int l, int s, int off)
{
    int m = tp_kal[s % 64];
    float f;
    tp_voice_t *v;
    if (!m)
        return;
    f = tp_mtof(m);
    v = tv_new(l, off);
    tv_osc(v, W_TRI, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 3.01f, 0.0f, 1.0f);
    tv_filter(v, F_LP, 3200.0f, 0.7f);
    tv_env_ad(v, 0.002f, 0.12f, 0.9f);
    tv_part_decay(v, 1, 0.035f, 0.15f, 0.12f, 0.9f);
    tv_len(v, 1.1f);
}

static void bass_line(int l, int s, int off)
{
    int p = s % 16, iv;
    float f, sps = tp_sps();
    tp_voice_t *v;
    if (p == 0 || p == 7)
        iv = 0;
    else if (p == 10)
        iv = 7;
    else if (p == 14)
        iv = 12;
    else
        return;
    f = tp_mtof(tp_chord_at(s)->root + iv);
    v = tv_new(l, off);
    tv_osc(v, W_TRI, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 0.5f, 0.0f, 1.0f);
    tv_filter(v, F_LP, 600.0f, 1.5f);
    tv_env_asr(v, 0.01f, 0.28f, sps * 2.5f, 0.25f);
    tv_len(v, sps * 2.5f + 0.4f);
}

static void tape_choir(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float bar = 16.0f * tp_sps();
    tp_voice_t *v;
    int i;
    if (s % 16)
        return;
    v = tv_new(l, off);                /* two notes, two detuned saws each, each saw its own vibrato; one low-pass */
    for (i = 0; i < 2; i++) {
        float f = tp_mtof(c->notes[1 + 2 * i] + 12);
        tv_osc(v, W_SAW, f, -9.0f, 1.0f);
        tv_osc(v, W_SAW, f, 9.0f, 1.0f);
    }
    for (i = 0; i < 4; i++)
        tv_lfo(v, LFO_DET, 4.6f + tp_rand() * 0.6f, 9.0f, 1 << i);
    tv_filter(v, F_LP, 1150.0f, 3.0f);
    tv_env_asr(v, 1.8f, 0.022f, bar - 1.8f, 2.5f);
    tv_len(v, bar + 2.8f);
}

/* ---- Beats ---- */
static void soft_kick(int l, int s, int off)
{
    int p = s % 16;
    tp_voice_t *v;
    if (p != 0 && p != 10)
        return;
    v = tv_new(l, off);
    tv_osc(v, W_SINE, 120.0f, 0.0f, 1.0f);
    tv_pitch_ramp(v, 42.0f, 0.14f);
    tv_env_ad(v, 0.002f, 0.8f, 0.55f);
    tv_len(v, 0.7f);
}

static void brushed_hats(int l, int s, int off)
{
    int open = s % 16 == 14, acc = s % 4 == 2;
    if (s % 2)
        return;
    tp_burst(l, off, F_HP, 7500.0f, 0.7f, 0.001f, acc ? 0.09f : 0.045f, open ? 0.25f : (acc ? 0.09f : 0.05f),
             open ? 0.35f : 0.14f);
}

static void rim_clicks(int l, int s, int off)
{
    int p = s % 16;
    tp_voice_t *v;
    if (p != 3 && p != 11 && !(p == 14 && s % 64 >= 48))
        return;
    v = tv_new(l, off);
    tv_osc(v, W_SINE, 1650.0f, 0.0f, 1.0f);
    tv_env_ad(v, 0.001f, 0.14f, 0.035f);
    tv_len(v, 0.1f);
    tp_burst(l, off, F_BP, 2500.0f, 4.0f, 0.001f, 0.1f, 0.02f, 0.06f);
}

static void brush_snare(int l, int s, int off)
{
    int p = s % 16, main = p == 4 || p == 12, ghost = p == 15 && tp_rand() < 0.35f;
    tp_voice_t *v;
    if (!main && !ghost)
        return;
    tp_burst(l, off, F_BP, 1900.0f, 0.7f, 0.004f, main ? 0.22f : 0.05f, main ? 0.28f : 0.1f, 0.45f);
    if (!main)
        return;
    v = tv_new(l, off);
    tv_osc(v, W_SINE, 185.0f, 0.0f, 1.0f);
    tv_pitch_ramp(v, 150.0f, 0.1f);
    tv_env_ad(v, 0.002f, 0.12f, 0.09f);
    tv_len(v, 0.2f);
}

static const tp_loop_t LOOPS[TP_NLOOP] = {
    /* name            fam  pan     rev    dly    fade  start        sched */
    { "Low drone",     0,  0.0f,  0.35f, 0.0f,  3.0f,  low_drone,   0 },
    { "Air pad",       0, -0.1f,  0.6f,  0.0f,  1.0f,  0,           air_pad },
    { "Shimmer",       0,  0.3f,  0.85f, 0.3f,  1.0f,  0,           shimmer },
    { "Sub pulse",     0,  0.0f,  0.15f, 0.0f,  0.5f,  0,           sub_pulse },
    { "Rain",          1, -0.2f,  0.5f,  0.0f,  2.0f,  rain_start,  rain },
    { "Wind",          1,  0.15f, 0.5f,  0.0f,  3.0f,  wind_start,  0 },
    { "Vinyl crackle", 1,  0.0f,  0.1f,  0.0f,  1.0f,  vinyl_start, vinyl },
    { "Ocean",         1, -0.1f,  0.4f,  0.0f,  3.0f,  ocean_start, 0 },
    { "Glass arp",     2, -0.3f,  0.5f,  0.4f,  0.3f,  0,           glass_arp },
    { "Kalimba",       2,  0.35f, 0.45f, 0.45f, 0.3f,  0,           kalimba },
    { "Bass line",     2,  0.0f,  0.1f,  0.0f,  0.3f,  0,           bass_line },
    { "Tape choir",    2, -0.15f, 0.75f, 0.0f,  1.0f,  0,           tape_choir },
    { "Soft kick",     3,  0.0f,  0.08f, 0.0f,  0.05f, 0,           soft_kick },
    { "Brushed hats",  3,  0.25f, 0.25f, 0.0f,  0.05f, 0,           brushed_hats },
    { "Rim clicks",    3, -0.35f, 0.35f, 0.5f,  0.05f, 0,           rim_clicks },
    { "Brush snare",   3,  0.1f,  0.55f, 0.0f,  0.05f, 0,           brush_snare },
};

const tp_bank_t TP_SHALLOWS = { "Shallows", LOOPS, CHORDS, P1, 9, 7, 0.24f, { "Drones", "Textures", "Melodies", "Beats" }, TP_STYLE_LUSH, 0 };
