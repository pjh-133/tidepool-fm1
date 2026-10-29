/* Tidepool on the FM-1: Bank 5, Dungeon Synth. A line-by-line port of BANK5_LOOPS in public/index.html.
 * Stone organ and Cold strings put a whole chord through one tremolo: here each voice of the chord has the same
 * tremolo, started together. Lost choir's two formant filters sit side by side. Cold strings' nine saws are two
 * voices (five and four); Tolling bell's seven partials are two voices, each partial with its own decay. */
#include "tp_voice.h"
#include "tp_math.h"

#define SR TP_SR
#define off_after(off, sec) ((off) + (int)((sec) * SR))

static const tp_chord_t CHORDS[4] = {
    { "Am", 45, { 45, 52, 57, 60, 64 } },
    { "F", 41, { 41, 48, 53, 57, 60 } },
    { "G", 43, { 43, 50, 55, 59, 62 } },
    { "Em", 40, { 40, 47, 52, 55, 59 } },
};
static const uint8_t P5[8] = { 69, 71, 72, 74, 76, 77, 79, 81 };

/* ---- Halls ---- */
static void stone_organ(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float bar = 16.0f * tp_sps();
    tp_voice_t *v;
    if (s % 16)
        return;
    v = tv_new(l, off);
    tv_osc(v, W_SQR, tp_mtof(c->notes[0] + 12), 0.0f, 1.0f);
    tv_osc(v, W_SQR, tp_mtof(c->notes[1] + 12), 0.0f, 1.0f);
    tv_osc(v, W_SQR, tp_mtof(c->notes[2] + 12), 0.0f, 1.0f);
    tv_osc(v, W_SQR, tp_mtof(c->notes[0] + 24), 0.0f, 1.0f);
    tv_amp(v, 0.8f);                         /* the chord's tremolo: 0.8, +-0.12 at 5.5 Hz */
    tv_lfo(v, LFO_AMP, 5.5f, 0.12f, 0);
    tv_filter(v, F_LP, 1100.0f, 0.8f);
    tv_env_asr(v, 0.5f, 0.028f, bar - 0.5f, 1.4f);
    tv_len(v, bar + 1.6f);
}

static void lost_choir(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float len = 32.0f * tp_sps();
    tp_voice_t *v;
    int i;
    if (s % 32)
        return;
    v = tv_new(l, off);
    for (i = 0; i < 3; i++) {
        tv_osc(v, W_SAW, tp_mtof(c->notes[2 + i] + 12), 0.0f, 1.0f);
        tv_lfo(v, LFO_DET, 4.4f + tp_rand() * 0.5f, 8.0f, 1 << i);
    }
    tv_filter(v, F_BP, 650.0f, 4.0f);        /* two vowel formants, side by side */
    tv_filter(v, F_BP, 1100.0f, 4.0f);
    tv_filter_parallel(v);
    tv_env_asr(v, 3.0f, 0.08f, len - 3.0f, 3.5f);
    tv_len(v, len + 3.7f);
}

static void cold_strings(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    const int pick[3] = { c->notes[1] + 12, c->notes[2] + 12, c->notes[4] + 12 };
    float bar = 16.0f * tp_sps();
    int h, k;
    if (s % 16)
        return;
    for (h = 0; h < 2; h++) {                /* three notes x three saws: voices of five and four */
        tp_voice_t *v = tv_new(l, off);
        for (k = h * 5; k < 9 && k < h * 5 + 5; k++)
            tv_osc(v, W_SAW, tp_mtof(pick[k / 3]), (float)(k % 3 - 1) * 9.0f, 1.0f);
        tv_amp(v, 0.75f);                    /* the chord's tremolo: 0.75, +-0.25 at 6.3 Hz */
        tv_lfo(v, LFO_AMP, 6.3f, 0.25f, 0);
        tv_filter(v, F_LP, 2200.0f, 0.7f);
        tv_env_asr(v, 1.4f, 0.02f, bar - 1.4f, 1.8f);
        tv_len(v, bar + 2.0f);
    }
}

static void dungeon_drone(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);
    tv_persist(v);
    tv_osc(v, W_SAW, tp_mtof(33), -6.0f, 0.5f);
    tv_osc(v, W_SAW, tp_mtof(33), 6.0f, 0.5f);
    tv_osc(v, W_SAW, tp_mtof(40), 0.0f, 0.25f);
    tv_osc(v, W_SINE, tp_mtof(21), 0.0f, 0.8f);
    tv_filter(v, F_LP, 260.0f, 1.5f);
    tv_env_const(v, 0.2f);
    tv_lfo(v, LFO_FC, 0.04f, 120.0f, 0);
}

/* ---- Depths ---- */
static void cave_drips(int l, int s, int off)
{
    float f;
    tp_voice_t *v;
    int at;
    if (tp_rand() > 0.07f)
        return;
    at = off_after(off, tp_rand() * tp_sps());
    f = 1300.0f + tp_rand() * 1400.0f;
    v = tv_new(l, at);
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_pitch_ramp(v, f * 0.6f, 0.12f);
    tv_env_ad(v, 0.002f, 0.09f, 0.18f);
    tv_len(v, 0.3f);
}

static void cold_wind_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);
    tv_persist(v);
    tv_noise(v, 1, 1.0f);
    tv_filter(v, F_BP, 400.0f, 2.0f);
    tv_env_const(v, 0.5f);
    tv_lfo(v, LFO_FC, 0.04f, 250.0f, 0);
    tv_lfo(v, LFO_GAIN, 0.06f, 0.25f, 0);
    v = tv_new(l, off);
    tv_persist(v);
    tv_noise(v, 0, 1.0f);
    tv_filter(v, F_BP, 900.0f, 15.0f);
    tv_env_const(v, 0.05f);
    tv_lfo(v, LFO_FC, 0.07f, 300.0f, 0);
}

static void torch_fire_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);
    tv_persist(v);
    tv_noise(v, 1, 1.0f);
    tv_filter(v, F_LP, 800.0f, 0.7f);
    tv_env_const(v, 0.22f);
}
static void torch_fire(int l, int s, int off)
{
    int n = tp_rand() < 0.5f ? 1 + (int)(tp_rand() * 3.0f) : 0, i;
    for (i = 0; i < n; i++) {
        int at = off_after(off, tp_rand() * tp_sps());
        float f = 1200.0f + tp_rand() * 4000.0f, pk = 0.03f + tp_rand() * 0.07f, d = 0.01f + tp_rand() * 0.03f;
        tp_burst(l, at, F_BP, f, 2.0f, 0.001f, pk, d, 0.06f);
    }
}

static void distant_thunder(int l, int s, int off)
{
    tp_voice_t *v;
    if (tp_rand() > 0.012f)
        return;
    v = tv_new(l, off);
    tv_noise(v, 1, 1.0f);
    tv_filter(v, F_LP, 170.0f, 0.7f);
    tv_env_asr(v, 0.9f, 0.5f, 1.2f, 3.5f);
    tv_len(v, 6.0f);
}

/* ---- Minstrels ---- */
static void harpsichord(int l, int s, int off)
{
    int m = tp_kal[s % 64];
    float f;
    tp_voice_t *v;
    if (!m)
        return;
    f = tp_mtof(m);
    v = tv_new(l, off);
    tv_osc(v, W_SAW, f, 0.0f, 1.0f);
    tv_osc(v, W_SAW, f * 2.0f, 0.0f, 0.5f);  /* the octave at half the level, the same decay */
    tv_filter(v, F_HP, 250.0f, 0.7f);
    tv_filter(v, F_LP, 4500.0f, 0.7f);
    tv_env_ad(v, 0.002f, 0.05f, 0.5f);
    tv_len(v, 0.7f);
}

static void wooden_flute(int l, int s, int off)
{
    float len = tp_sps() * 6.0f;
    int m;
    tp_voice_t *v;
    if (s % 16 != 8 || tp_rand() > 0.55f)
        return;
    m = P5[(int)(tp_rand() * 8.0f)];
    v = tv_new(l, off);
    tv_osc(v, W_SINE, tp_mtof(m), 0.0f, 1.0f);
    tv_lfo(v, LFO_DET, 4.8f, 6.0f, 1);
    tv_env_asr(v, 0.2f, 0.08f, len, 0.8f);
    tv_len(v, len + 1.0f);
    tp_burst(l, off, F_BP, 2600.0f, 1.2f, 0.12f, 0.02f, 0.4f, 0.6f);
}

static void lute(int l, int s, int off)
{
    int idx;
    float f;
    tp_voice_t *v;
    switch (s % 16) {
    case 0: idx = 0; break;
    case 4: idx = 2; break;
    case 6: idx = 3; break;
    case 8: idx = 1; break;
    case 12: idx = 4; break;
    case 14: idx = 2; break;
    default: return;
    }
    f = tp_mtof(tp_chord_at(s)->notes[idx] + 12);
    v = tv_new(l, off);
    tv_osc(v, W_TRI, f, 0.0f, 1.0f);
    tv_osc(v, W_SQR, f, 0.0f, 1.0f);         /* the pluck: a square that dies in a moment */
    tv_filter(v, F_LP, 1800.0f, 0.7f);
    tv_env_ad(v, 0.002f, 0.1f, 0.9f);
    tv_part_decay(v, 1, 0.02f, 0.12f, 0.1f, 0.9f);
    tv_len(v, 1.1f);
}

static void tolling_bell(int l, int s, int off)
{
    static const float R[7] = { 0.5f, 1.0f, 1.19f, 1.5f, 2.0f, 2.76f, 4.2f };
    static const float LV[7] = { 0.5f, 1.0f, 0.6f, 0.4f, 0.5f, 0.3f, 0.15f };
    float f;
    int h, i;
    if (s % 32 != 0)
        return;
    f = tp_mtof(tp_chord_at(s)->root + 12);
    for (h = 0; h < 2; h++) {                /* partials 0-3 and 4-6: each voice's envelope is its first's */
        int a = h ? 4 : 0, b = h ? 7 : 4;
        float pk = 0.06f * LV[a], d = 4.5f / (R[a] + 0.5f);
        tp_voice_t *v = tv_new(l, off);
        for (i = a; i < b; i++)
            tv_osc(v, W_SINE, f * R[i], 0.0f, 1.0f);
        tv_env_ad(v, 0.003f, pk, d);
        for (i = a + 1; i < b; i++)
            tv_part_decay(v, i - a, 0.06f * LV[i], 4.5f / (R[i] + 0.5f), pk, d);
        tv_len(v, 5.0f);
    }
}

/* ---- Rites ---- */
static void war_drum(int l, int s, int off)
{
    int p = s % 16;
    float d = p == 0 ? 0.85f : p == 8 ? 0.55f : (p == 11 && tp_rand() < 0.5f) ? 0.3f : 0.0f;
    tp_voice_t *v;
    if (d == 0.0f)
        return;
    v = tv_new(l, off);
    tv_osc(v, W_SINE, 95.0f, 0.0f, 1.0f);
    tv_pitch_ramp(v, 42.0f, 0.2f);
    tv_env_ad(v, 0.003f, d, 0.8f);
    tv_len(v, 0.9f);
    tp_burst(l, off, F_LP, 300.0f, 0.7f, 0.001f, d * 0.4f, 0.1f, 0.2f);
}

static void chain_rattle(int l, int s, int off)
{
    int i;
    if (s % 16 != 14 || tp_rand() > 0.55f)
        return;
    for (i = 0; i < 5; i++) {
        int at = off_after(off, i * 0.028f + tp_rand() * 0.01f);
        tp_burst(l, at, F_BP, 4500.0f + tp_rand() * 3500.0f, 10.0f, 0.001f, 0.06f, 0.09f, 0.15f);
    }
}

static void anvil(int l, int s, int off)
{
    int p = s % 16;
    float acc;
    tp_voice_t *v;
    if (p != 4 && p != 12)
        return;
    acc = p == 4 ? 1.0f : 0.6f;
    v = tv_new(l, off);                      /* three partials, each its own level and decay */
    tv_osc(v, W_SINE, 880.0f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, 2380.0f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, 3790.0f, 0.0f, 1.0f);
    tv_env_ad(v, 0.001f, 0.06f * acc, 0.4f);
    tv_part_decay(v, 1, 0.036f * acc, 0.3f, 0.06f * acc, 0.4f);
    tv_part_decay(v, 2, 0.021f * acc, 0.2f, 0.06f * acc, 0.4f);
    tv_len(v, 0.5f);
    tp_burst(l, off, F_BP, 3000.0f, 2.0f, 0.001f, 0.06f * acc, 0.015f, 0.04f);
}

static void marching_feet(int l, int s, int off)
{
    int acc = s % 8 == 0;
    if (s % 4)
        return;
    tp_burst(l, off, F_LP, 260.0f, 0.7f, 0.004f, acc ? 0.32f : 0.2f, 0.08f, 0.16f);
    tp_burst(l, off, F_BP, 1900.0f, 1.0f, 0.001f, 0.03f, 0.03f, 0.06f);
}

static const tp_loop_t LOOPS[TP_NLOOP] = {
    /* name               fam  pan     rev    dly    fade  start             sched */
    { "Stone organ",      0,  0.0f,  0.75f, 0.0f,  1.5f,  0,                stone_organ },
    { "Lost choir",       0, -0.2f,  0.9f,  0.0f,  2.0f,  0,                lost_choir },
    { "Cold strings",     0,  0.15f, 0.6f,  0.0f,  1.2f,  0,                cold_strings },
    { "Dungeon drone",    0,  0.0f,  0.5f,  0.0f,  4.0f,  dungeon_drone,    0 },
    { "Cave drips",       1,  0.1f,  0.9f,  0.6f,  0.2f,  0,                cave_drips },
    { "Cold wind",        1, -0.15f, 0.6f,  0.0f,  3.0f,  cold_wind_start,  0 },
    { "Torch fire",       1,  0.2f,  0.35f, 0.0f,  1.0f,  torch_fire_start, torch_fire },
    { "Distant thunder",  1,  0.0f,  0.7f,  0.0f,  0.5f,  0,                distant_thunder },
    { "Harpsichord",      2, -0.2f,  0.5f,  0.3f,  0.3f,  0,                harpsichord },
    { "Wooden flute",     2,  0.25f, 0.7f,  0.35f, 0.5f,  0,                wooden_flute },
    { "Lute",             2, -0.1f,  0.45f, 0.25f, 0.3f,  0,                lute },
    { "Tolling bell",     2, -0.2f,  0.85f, 0.3f,  0.2f,  0,                tolling_bell },
    { "War drum",         3,  0.0f,  0.6f,  0.0f,  0.05f, 0,                war_drum },
    { "Chain rattle",     3,  0.3f,  0.5f,  0.0f,  0.05f, 0,                chain_rattle },
    { "Anvil",            3, -0.3f,  0.5f,  0.2f,  0.05f, 0,                anvil },
    { "Marching feet",    3,  0.1f,  0.5f,  0.0f,  0.05f, 0,                marching_feet },
};

const tp_bank_t TP_DUNGEON = { "Dungeon Synth", LOOPS, CHORDS, P5, 8, 31, 0.2f, { "Halls", "Depths", "Minstrels", "Rites" }, TP_STYLE_TRIAD, 0 };
