/* Tidepool on the FM-1: Bank 3, Forest Floor. A line-by-line port of BANK3_LOOPS in public/index.html.
 * Warm organ's eight oscillators (four notes, each a detuned triangle and a sine an octave up) are two voices of
 * four; partials with their own decay (marimba, nylon guitar) are one voice with a faster-decaying partial. */
#include "tp_voice.h"
#include "tp_math.h"

#define SR TP_SR
#define off_after(off, sec) ((off) + (int)((sec) * SR))

static const tp_chord_t CHORDS[4] = {
    { "Gmaj9", 43, { 55, 59, 62, 66, 69 } },
    { "Em7", 40, { 52, 55, 59, 62, 67 } },
    { "Cadd9", 36, { 52, 55, 60, 62, 67 } },
    { "Dsus4", 38, { 50, 55, 57, 62, 67 } },
};
static const uint8_t P3[8] = { 67, 69, 71, 74, 76, 79, 81, 83 };

/* ---- Ground ---- */
static void earth_hum(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);
    tv_persist(v);
    tv_osc(v, W_SINE, tp_mtof(31), 0.0f, 0.9f);
    tv_osc(v, W_SINE, tp_mtof(38), 0.0f, 0.5f);
    tv_osc(v, W_TRI, tp_mtof(43), 0.0f, 0.2f);
    tv_filter(v, F_LP, 300.0f, 1.0f);
    tv_env_const(v, 0.2f);
    tv_lfo(v, LFO_FC, 0.05f, 60.0f, 0);
}

static void warm_organ(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float bar = 16.0f * tp_sps();
    int h, i;
    if (s % 16)
        return;
    for (h = 0; h < 2; h++) {                /* four notes: two voices of two notes each */
        tp_voice_t *v = tv_new(l, off);
        for (i = h * 2; i < h * 2 + 2; i++) {
            tv_osc(v, W_TRI, tp_mtof(c->notes[i]), i % 2 ? -4.0f : 4.0f, 1.0f);
            tv_osc(v, W_SINE, tp_mtof(c->notes[i] + 12), 0.0f, 1.0f);
        }
        tv_filter(v, F_LP, 900.0f, 0.7f);
        tv_env_asr(v, 1.0f, 0.04f, bar - 1.0f, 1.5f);
        tv_len(v, bar + 1.7f);
    }
}

static void cello(int l, int s, int off)
{
    float len = 32.0f * tp_sps(), f;
    tp_voice_t *v;
    if (s % 32)
        return;
    f = tp_mtof(tp_chord_at(s)->root + 12);
    v = tv_new(l, off);
    tv_osc(v, W_SAW, f, -6.0f, 1.0f);
    tv_osc(v, W_SAW, f, 6.0f, 1.0f);
    tv_lfo(v, LFO_DET, 5.2f, 8.0f, 3);       /* both saws' vibrato: the same rate, started together */
    tv_filter(v, F_LP, 700.0f, 1.0f);
    tv_env_asr(v, 0.9f, 0.07f, len - 0.9f, 1.5f);
    tv_len(v, len + 1.7f);
}

static void root_pulse(int l, int s, int off)
{
    int p = s % 16;
    tp_voice_t *v;
    if (p != 0 && p != 8)
        return;
    v = tv_new(l, off);
    tv_osc(v, W_SINE, tp_mtof(tp_chord_at(s)->root + (p ? 7 : 0)), 0.0f, 1.0f);
    tv_env_ad(v, 0.12f, 0.3f, 1.4f);
    tv_len(v, 1.7f);
}

/* ---- Woods ---- */
static void crickets(int l, int s, int off)
{
    float f;
    int t0, i;
    if (tp_rand() > 0.1f)
        return;
    f = 4300.0f + tp_rand() * 300.0f;
    t0 = off_after(off, tp_rand() * tp_sps());
    for (i = 0; i < 3; i++) {
        tp_voice_t *v = tv_new(l, off_after(t0, i * 0.055f));
        tv_osc(v, W_SINE, f, 0.0f, 1.0f);
        tv_env_ad(v, 0.004f, 0.03f, 0.03f);
        tv_len(v, 0.06f);
    }
}

static void stream_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);
    tv_persist(v);
    tv_noise(v, 0, 1.0f);
    tv_filter(v, F_BP, 1400.0f, 0.8f);
    tv_env_const(v, 0.1f);
    v = tv_new(l, off);
    tv_persist(v);
    tv_noise(v, 0, 1.0f);
    tv_filter(v, F_BP, 2200.0f, 3.0f);
    tv_env_const(v, 0.05f);
    tv_lfo(v, LFO_FC, 0.7f, 600.0f, 0);
    tv_lfo(v, LFO_FC, 0.9f, 400.0f, 0);
    tv_lfo(v, LFO_GAIN, 0.11f, 0.03f, 0);
}

static void leaf_rustle(int l, int s, int off)
{
    int n = tp_rand() < 0.5f ? 1 + (tp_rand() < 0.4f ? 1 : 0) : 0, i;
    for (i = 0; i < n; i++) {
        int at = off_after(off, tp_rand() * tp_sps());
        tp_burst(l, at, F_BP, 3500.0f + tp_rand() * 1500.0f, 0.9f, 0.02f, 0.05f, 0.09f, 0.18f);
    }
}

static void owl(int l, int s, int off)
{
    int i;
    if (s % 64 != 40 || tp_rand() > 0.8f)
        return;
    for (i = 0; i < 2; i++) {                /* hoo ... hoo, each falling a tone */
        tp_voice_t *v = tv_new(l, off_after(off, i * 0.55f));
        tv_osc(v, W_SINE, tp_mtof(i ? 59 : 62), 0.0f, 1.0f);
        tv_pitch_ramp(v, tp_mtof(i ? 57 : 60), 0.4f);
        tv_env_ad(v, 0.07f, 0.14f, 0.4f);
        tv_len(v, 0.6f);
    }
}

/* ---- Songs ---- */
static void marimba(int l, int s, int off)
{
    int m = tp_kal[s % 64];
    float f;
    tp_voice_t *v;
    if (!m)
        return;
    f = tp_mtof(m - 12);
    v = tv_new(l, off);
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 4.0f, 0.0f, 1.0f);
    tv_env_ad(v, 0.002f, 0.16f, 0.5f);
    tv_part_decay(v, 1, 0.04f, 0.08f, 0.16f, 0.5f);
    tv_len(v, 0.7f);
}

static void bamboo_flute(int l, int s, int off)
{
    float len = tp_sps() * 3.0f;
    int m;
    tp_voice_t *v;
    if (s % 8 || tp_rand() > 0.45f)
        return;
    m = P3[3 + (int)(tp_rand() * 5.0f)];
    v = tv_new(l, off);
    tv_osc(v, W_SINE, tp_mtof(m), 0.0f, 1.0f);
    tv_lfo(v, LFO_DET, 4.6f, 5.0f, 1);
    tv_env_asr(v, 0.12f, 0.07f, len, 0.5f);
    tv_len(v, len + 0.7f);
    tp_burst(l, off, F_BP, 3000.0f, 1.2f, 0.08f, 0.02f, 0.3f, 0.5f);
}

static void nylon_guitar(int l, int s, int off)
{
    int p = s % 16, idx;
    float f;
    tp_voice_t *v;
    switch (p) {
    case 0: idx = 0; break;
    case 3: idx = 2; break;
    case 6: idx = 3; break;
    case 8: idx = 1; break;
    case 11: idx = 4; break;
    case 14: idx = 3; break;
    default: return;
    }
    f = tp_mtof(tp_chord_at(s)->notes[idx]);
    v = tv_new(l, off);
    tv_osc(v, W_TRI, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 2.0f, 0.0f, 1.0f);
    tv_filter(v, F_LP, 2400.0f, 0.7f);
    tv_env_ad(v, 0.002f, 0.1f, 1.1f);
    tv_part_decay(v, 1, 0.03f, 0.3f, 0.1f, 1.1f);
    tv_len(v, 1.3f);
}

static void music_box(int l, int s, int off)
{
    tp_voice_t *v;
    if (s % 4 || tp_rand() > 0.35f)
        return;
    v = tv_new(l, off);
    tv_osc(v, W_SINE, tp_mtof(P3[(int)(tp_rand() * 8.0f)] + 12), 0.0f, 1.0f);
    tv_env_ad(v, 0.001f, 0.05f, 1.6f);
    tv_len(v, 1.8f);
}

/* ---- Steps ---- */
static void hand_drum(int l, int s, int off)
{
    float f0, f1, pk;
    tp_voice_t *v;
    switch (s % 16) {
    case 0: f0 = 140.0f; f1 = 90.0f; pk = 0.42f; break;
    case 6: f0 = 240.0f; f1 = 170.0f; pk = 0.2f; break;
    case 10: f0 = 240.0f; f1 = 170.0f; pk = 0.16f; break;
    case 13: f0 = 200.0f; f1 = 150.0f; pk = 0.08f; break;
    default: return;
    }
    v = tv_new(l, off);
    tv_osc(v, W_SINE, f0, 0.0f, 1.0f);
    tv_pitch_ramp(v, f1, 0.08f);
    tv_env_ad(v, 0.002f, pk, 0.3f);
    tv_len(v, 0.4f);
    tp_burst(l, off, F_BP, 1800.0f, 2.0f, 0.001f, pk * 0.25f, 0.03f, 0.08f);
}

static void shaker(int l, int s, int off)
{
    if (s % 2)
        return;
    tp_burst(l, off, F_BP, 6500.0f, 1.0f, 0.012f, s % 4 == 0 ? 0.08f : 0.045f, 0.06f, 0.14f);
}

static void wood_block(int l, int s, int off)
{
    int p = s % 16;
    tp_voice_t *v;
    if (p != 0 && p != 3 && p != 6 && p != 10 && p != 12)
        return;
    v = tv_new(l, off);
    tv_osc(v, W_SINE, 1050.0f, 0.0f, 1.0f);
    tv_env_ad(v, 0.001f, 0.12f, 0.05f);
    tv_len(v, 0.1f);
}

static void foot_stomp(int l, int s, int off)
{
    int p = s % 16;
    tp_voice_t *v;
    if (p != 0 && p != 8)
        return;
    v = tv_new(l, off);
    tv_osc(v, W_SINE, 90.0f, 0.0f, 1.0f);
    tv_pitch_ramp(v, 50.0f, 0.15f);
    tv_env_ad(v, 0.002f, 0.7f, 0.3f);
    tv_len(v, 0.4f);
    tp_burst(l, off, F_LP, 400.0f, 0.7f, 0.001f, 0.3f, 0.08f, 0.15f);
}

static const tp_loop_t LOOPS[TP_NLOOP] = {
    /* name             fam  pan     rev    dly    fade  start         sched */
    { "Earth hum",      0,  0.0f,  0.3f,  0.0f,  4.0f,  earth_hum,    0 },
    { "Warm organ",     0, -0.1f,  0.5f,  0.0f,  1.0f,  0,            warm_organ },
    { "Cello",          0,  0.1f,  0.55f, 0.0f,  1.0f,  0,            cello },
    { "Root pulse",     0,  0.0f,  0.2f,  0.0f,  0.5f,  0,            root_pulse },
    { "Crickets",       1,  0.2f,  0.2f,  0.0f,  1.0f,  0,            crickets },
    { "Stream",         1, -0.15f, 0.3f,  0.0f,  3.0f,  stream_start, 0 },
    { "Leaf rustle",    1,  0.0f,  0.35f, 0.0f,  0.5f,  0,            leaf_rustle },
    { "Owl",            1, -0.3f,  0.7f,  0.3f,  0.2f,  0,            owl },
    { "Marimba",        2, -0.2f,  0.4f,  0.35f, 0.3f,  0,            marimba },
    { "Bamboo flute",   2,  0.25f, 0.65f, 0.3f,  0.5f,  0,            bamboo_flute },
    { "Nylon guitar",   2, -0.1f,  0.35f, 0.25f, 0.3f,  0,            nylon_guitar },
    { "Music box",      2,  0.35f, 0.6f,  0.5f,  0.3f,  0,            music_box },
    { "Hand drum",      3, -0.1f,  0.35f, 0.0f,  0.05f, 0,            hand_drum },
    { "Shaker",         3,  0.3f,  0.25f, 0.0f,  0.05f, 0,            shaker },
    { "Wood block",     3, -0.3f,  0.3f,  0.3f,  0.05f, 0,            wood_block },
    { "Foot stomp",     3,  0.0f,  0.2f,  0.0f,  0.05f, 0,            foot_stomp },
};

const tp_bank_t TP_FORESTFLOOR = { "Forest Floor", LOOPS, CHORDS, P3, 8, 5, 0.26f,
                                   { "Ground", "Woods", "Songs", "Steps" }, TP_STYLE_FOLK, 0 };
