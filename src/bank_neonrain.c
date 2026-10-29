/* Tidepool on the FM-1: Bank 4, Neon Rain. A line-by-line port of BANK4_LOOPS in public/index.html.
 * Juno pad's ten saws are two voices of five; Tram bell's three partials are one voice with faster-decaying
 * partials. Whistle echo's short linear pitch rise is exponential here (two semitones: the same). */
#include "tp_voice.h"
#include "tp_math.h"

#define SR TP_SR
#define off_after(off, sec) ((off) + (int)((sec) * SR))

static const tp_chord_t CHORDS[4] = {
    { "Am9", 45, { 57, 60, 64, 67, 71 } },
    { "Fmaj9", 41, { 53, 57, 60, 64, 67 } },
    { "Cmaj9", 36, { 48, 52, 55, 59, 62 } },
    { "G6/9", 43, { 55, 59, 62, 64, 69 } },
};
static const uint8_t P4[7] = { 69, 72, 74, 76, 79, 81, 84 };

/* ---- Synths ---- */
static void juno_pad(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float bar = 16.0f * tp_sps();
    int h, k;
    if (s % 16)
        return;
    for (h = 0; h < 2; h++) {                /* five notes, two detuned saws each: two voices of five */
        tp_voice_t *v = tv_new(l, off);
        for (k = h * 5; k < h * 5 + 5; k++)
            tv_osc(v, W_SAW, tp_mtof(c->notes[k / 2]), k % 2 ? 10.0f : -10.0f, 1.0f);
        tv_filter(v, F_LP, 1800.0f, 0.7f);
        tv_env_asr(v, 0.9f, 0.03f, bar - 0.9f, 1.8f);
        tv_len(v, bar + 2.0f);
    }
}

static void chord_stabs(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    int p = s % 16, i;
    tp_voice_t *v;
    if (p != 6 && p != 13)
        return;
    v = tv_new(l, off);
    for (i = 0; i < 4; i++)
        tv_osc(v, W_SAW, tp_mtof(c->notes[i]), 0.0f, 1.0f);
    tv_filter(v, F_LP, 3200.0f, 3.0f);
    tv_filter_ramp(v, 1, 450.0f, 0.45f);
    tv_env_ad(v, 0.004f, 0.035f, 0.6f);
    tv_len(v, 0.7f);
}

static void dark_sub(int l, int s, int off)
{
    int iv;
    float f, len = tp_sps() * 2.0f;
    tp_voice_t *v;
    switch (s % 16) {
    case 0: case 3: case 10: iv = 0; break;
    case 6: iv = 7; break;
    case 14: iv = 12; break;
    default: return;
    }
    f = tp_mtof(tp_chord_at(s)->root - 12 + iv);
    v = tv_new(l, off);
    tv_osc(v, W_TRI, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_filter(v, F_LP, 500.0f, 1.2f);
    tv_env_asr(v, 0.01f, 0.3f, len, 0.2f);
    tv_len(v, len + 0.4f);
}

static void bus_hum(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);
    tv_persist(v);
    tv_osc(v, W_SAW, 55.0f, 0.0f, 1.0f);
    tv_osc(v, W_SAW, 55.4f, 0.0f, 1.0f);
    tv_filter(v, F_LP, 220.0f, 1.0f);
    tv_env_const(v, 0.14f);
    tv_lfo(v, LFO_FC, 0.22f, 60.0f, 0);
}

/* ---- City ---- */
static void rain_glass_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);
    tv_persist(v);
    tv_noise(v, 0, 1.0f);
    tv_filter(v, F_HP, 2000.0f, 0.7f);
    tv_filter(v, F_LP, 9000.0f, 0.7f);
    tv_env_const(v, 0.06f);
}
static void rain_glass(int l, int s, int off)
{
    int n = tp_rand() < 0.5f ? 1 + (tp_rand() < 0.3f ? 1 : 0) : 0, i;
    for (i = 0; i < n; i++) {
        int at = off_after(off, tp_rand() * tp_sps());
        float f = 2500.0f + tp_rand() * 5000.0f, pk = 0.05f + tp_rand() * 0.08f, d = 0.025f + tp_rand() * 0.03f;
        tp_burst(l, at, F_BP, f, 8.0f, 0.001f, pk, d, 0.08f);
    }
}

static void traffic_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);
    tv_persist(v);
    tv_noise(v, 1, 1.0f);
    tv_filter(v, F_LP, 500.0f, 0.6f);
    tv_env_const(v, 0.5f);
    tv_lfo(v, LFO_GAIN, 0.06f, 0.25f, 0);
}
static void traffic(int l, int s, int off)
{
    tp_voice_t *v;
    if (tp_rand() > 0.014f)
        return;
    v = tv_new(l, off);                      /* a car passing: a band of noise that rises and falls */
    tv_noise(v, 0, 1.0f);
    tv_filter(v, F_BP, 300.0f, 2.0f);
    tv_filter_ramp(v, 0, 950.0f, 1.2f);
    tv_filter_ramp(v, 0, 260.0f, 1.6f);
    tv_env_asr(v, 1.1f, 0.12f, 0.3f, 1.4f);
    tv_len(v, 2.9f);
}

static void neon_buzz_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);
    tv_persist(v);
    tv_osc(v, W_SAW, 50.0f, 0.0f, 1.0f);
    tv_filter(v, F_LP, 700.0f, 0.7f);
    tv_env_const(v, 0.02f);
    tv_lfo(v, LFO_GAIN, 0.35f, 0.008f, 0);
}
static void neon_buzz(int l, int s, int off)
{
    if (tp_rand() > 0.03f)
        return;
    tp_burst(l, off_after(off, tp_rand() * tp_sps()), F_HP, 3000.0f, 0.7f, 0.001f, 0.05f, 0.02f, 0.05f);
}

static void tram_bell(int l, int s, int off)
{
    int i;
    if (s % 64 != 52 || tp_rand() > 0.6f)
        return;
    for (i = 0; i < 2; i++) {                /* ding ... ding: three partials each */
        tp_voice_t *v = tv_new(l, off_after(off, i * 0.32f));
        tv_osc(v, W_SINE, 1320.0f, 0.0f, 1.0f);
        tv_osc(v, W_SINE, 1320.0f * 2.4f, 0.0f, 1.0f);
        tv_osc(v, W_SINE, 1320.0f * 5.95f, 0.0f, 1.0f);
        tv_env_ad(v, 0.002f, 0.05f, 2.1f);
        tv_part_decay(v, 1, 0.02f, 1.8f / 2.4f + 0.3f, 0.05f, 2.1f);
        tv_part_decay(v, 2, 0.01f, 1.8f / 5.95f + 0.3f, 0.05f, 2.1f);
        tv_len(v, 2.2f);
    }
}

/* ---- Leads ---- */
static void saw_lead(int l, int s, int off)
{
    float len = tp_sps() * 10.0f, f;
    tp_voice_t *v;
    if (s % 16 || tp_rand() > 0.55f)
        return;
    f = tp_mtof(P4[(int)(tp_rand() * 7.0f)]);
    v = tv_new(l, off);
    tv_osc(v, W_SAW, f, -7.0f, 1.0f);
    tv_osc(v, W_SAW, f, 7.0f, 1.0f);
    tv_lfo(v, LFO_DET, 5.3f, 6.0f, 3);       /* both saws' vibrato: the same rate, started together */
    tv_filter(v, F_LP, 1500.0f, 2.0f);
    tv_env_asr(v, 0.05f, 0.04f, len, 0.6f);
    tv_len(v, len + 0.7f);
}

static void pluck_arp(int l, int s, int off)
{
    static const int8_t pat[8] = { 0, 2, 4, 2, 3, 1, 4, -1 };
    int idx;
    tp_voice_t *v;
    if (s % 2)
        return;
    idx = pat[(s / 2) % 8];
    if (idx < 0)
        return;
    v = tv_new(l, off);
    tv_osc(v, W_SAW, tp_mtof(tp_chord_at(s)->notes[idx] + 12), 0.0f, 1.0f);
    tv_filter(v, F_LP, 3000.0f, 2.0f);
    tv_filter_ramp(v, 1, 500.0f, 0.25f);
    tv_env_ad(v, 0.002f, 0.05f, 0.3f);
    tv_len(v, 0.4f);
}

static void e_piano(int l, int s, int off)
{
    int idx;
    float f;
    tp_voice_t *v;
    switch (s % 16) {
    case 0: idx = 1; break;
    case 6: idx = 3; break;
    case 10: idx = 2; break;
    case 14: idx = 4; break;
    default: return;
    }
    if (tp_rand() > 0.85f)
        return;
    f = tp_mtof(tp_chord_at(s)->notes[idx] + 12);
    v = tv_new(l, off);
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_fm(v, 1.0f, f * 1.1f, 0.5f);
    tv_env_ad(v, 0.003f, 0.07f, 1.7f);
    tv_len(v, 1.9f);
}

static void whistle_echo(int l, int s, int off)
{
    int m;
    tp_voice_t *v;
    if (s % 16 != 2 || tp_rand() > 0.4f)
        return;
    m = P4[(int)(tp_rand() * 7.0f)] + 12;
    v = tv_new(l, off);
    tv_osc(v, W_SINE, tp_mtof(m - 2), 0.0f, 1.0f);
    tv_pitch_ramp(v, tp_mtof(m), 0.12f);
    tv_lfo(v, LFO_DET, 6.0f, 9.0f, 1);
    tv_env_asr(v, 0.06f, 0.05f, 0.9f, 0.7f);
    tv_len(v, 1.8f);
}

/* ---- Drums ---- */
static void lofi_kick(int l, int s, int off)
{
    int p = s % 16;
    tp_voice_t *v;
    if (p != 0 && p != 7 && p != 10)
        return;
    v = tv_new(l, off);
    tv_osc(v, W_SINE, 100.0f, 0.0f, 1.0f);
    tv_pitch_ramp(v, 45.0f, 0.12f);
    tv_env_ad(v, 0.002f, p == 7 ? 0.4f : 0.75f, 0.5f);
    tv_len(v, 0.6f);
}

static void snare_crack(int l, int s, int off)
{
    int p = s % 16;
    tp_voice_t *v;
    if (p != 4 && p != 12)
        return;
    tp_burst(l, off, F_BP, 2200.0f, 0.9f, 0.002f, 0.2f, 0.16f, 0.3f);
    v = tv_new(l, off);
    tv_osc(v, W_SINE, 200.0f, 0.0f, 1.0f);
    tv_pitch_ramp(v, 160.0f, 0.08f);
    tv_env_ad(v, 0.002f, 0.14f, 0.1f);
    tv_len(v, 0.2f);
}

static void shuffle_hats(int l, int s, int off)
{
    int at = s % 4 == 2 ? off_after(off, tp_sps() * 0.3f) : off, acc = s % 8 == 0;
    if (s % 2)
        return;
    tp_burst(l, at, F_HP, 8500.0f, 0.7f, 0.001f, acc ? 0.08f : 0.04f, 0.05f, 0.12f);
}

static void clap_verb(int l, int s, int off)
{
    static const float dt[3] = { 0.0f, 0.011f, 0.023f };
    int p = s % 16, i;
    if (p != 12 && !(p == 7 && tp_rand() < 0.25f))
        return;
    for (i = 0; i < 3; i++)
        tp_burst(l, off_after(off, dt[i]), F_BP, 1250.0f, 1.3f, 0.001f, 0.11f, 0.02f, 0.06f);
    tp_burst(l, off_after(off, 0.03f), F_BP, 1100.0f, 1.0f, 0.001f, 0.1f, 0.22f, 0.3f);
}

static const tp_loop_t LOOPS[TP_NLOOP] = {
    /* name             fam  pan     rev    dly    fade  start             sched */
    { "Juno pad",       0, -0.1f,  0.6f,  0.0f,  1.0f,  0,                juno_pad },
    { "Chord stabs",    0,  0.2f,  0.4f,  0.5f,  0.1f,  0,                chord_stabs },
    { "Dark sub",       0,  0.0f,  0.1f,  0.0f,  0.3f,  0,                dark_sub },
    { "Bus hum",        0,  0.0f,  0.2f,  0.0f,  3.0f,  bus_hum,          0 },
    { "Rain on glass",  1, -0.2f,  0.4f,  0.0f,  2.0f,  rain_glass_start, rain_glass },
    { "Traffic",        1,  0.15f, 0.5f,  0.0f,  3.0f,  traffic_start,    traffic },
    { "Neon buzz",      1,  0.3f,  0.2f,  0.0f,  1.0f,  neon_buzz_start,  neon_buzz },
    { "Tram bell",      1, -0.3f,  0.6f,  0.5f,  0.2f,  0,                tram_bell },
    { "Saw lead",       2,  0.1f,  0.6f,  0.4f,  0.3f,  0,                saw_lead },
    { "Pluck arp",      2, -0.25f, 0.4f,  0.45f, 0.3f,  0,                pluck_arp },
    { "E-piano",        2,  0.2f,  0.5f,  0.3f,  0.3f,  0,                e_piano },
    { "Whistle echo",   2, -0.1f,  0.7f,  0.7f,  0.3f,  0,                whistle_echo },
    { "Lo-fi kick",     3,  0.0f,  0.1f,  0.0f,  0.05f, 0,                lofi_kick },
    { "Snare crack",    3,  0.1f,  0.5f,  0.0f,  0.05f, 0,                snare_crack },
    { "Shuffle hats",   3,  0.25f, 0.25f, 0.0f,  0.05f, 0,                shuffle_hats },
    { "Clap verb",      3, -0.2f,  0.85f, 0.0f,  0.05f, 0,                clap_verb },
};

const tp_bank_t TP_NEONRAIN = { "Neon Rain", LOOPS, CHORDS, P4, 7, 13, 0.22f, { "Synths", "City", "Leads", "Drums" }, TP_STYLE_LUSH, 0 };
