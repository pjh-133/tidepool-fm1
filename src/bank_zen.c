/* Tidepool on the FM-1: Bank 8, Zen Garden. Written for the FM-1 (no web app original): a temple garden in spring
 * rain, in F major. A temple drone, a sho's held clusters, a singing bowl and a low koto; the bamboo fountain, rain on
 * leaves, wind chimes and cicadas; koto, shakuhachi, a temple bell and water drops in a stone basin; taiko, wooden
 * clappers, a small bell and the rake in the sand. */
#include "tp_voice.h"
#include "tp_math.h"

#define SR TP_SR
#define off_after(off, sec) ((off) + (int)((sec) * SR))

static const tp_chord_t CHORDS[4] = {
    { "Fadd9", 41, { 53, 57, 60, 65, 67 } },
    { "Am7", 33, { 52, 55, 57, 60, 64 } },
    { "Bbadd9", 34, { 50, 53, 58, 60, 65 } },
    { "Csus4", 36, { 53, 55, 60, 65, 67 } },
};
static const uint8_t P8[8] = { 65, 67, 69, 72, 74, 77, 79, 81 };   /* F major pentatonic */

/* ---- Stillness ---- */
static void temple_drone(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);           /* F and C, two sines each a breath apart: a slow beating */
    tv_persist(v);
    tv_osc(v, W_SINE, tp_mtof(29), 0.0f, 1.0f);
    tv_osc(v, W_SINE, tp_mtof(36), -3.0f, 0.5f);
    tv_osc(v, W_SINE, tp_mtof(36), 3.0f, 0.5f);
    tv_osc(v, W_TRI, tp_mtof(41), 0.0f, 0.15f);
    tv_filter(v, F_LP, 500.0f, 0.7f);
    tv_env_const(v, 0.16f);
}

static void sho(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float len = 32.0f * tp_sps();
    tp_voice_t *v;
    int i;
    if (s % 32)
        return;
    v = tv_new(l, off);                       /* the mouth organ: a reedy cluster that swells in and out */
    for (i = 0; i < 5; i++)
        tv_osc(v, i % 2 ? W_SQR : W_TRI, tp_mtof(c->notes[i] + 12), (float)(i * 2 - 4), i % 2 ? 0.35f : 1.0f);
    tv_filter(v, F_LP, 1600.0f, 0.7f);
    tv_env_asr(v, len * 0.35f, 0.055f, len * 0.3f, len * 0.4f);
    tv_len(v, len * 1.1f);
}

static void singing_bowl(int l, int s, int off)
{
    float f;
    tp_voice_t *v;
    if (s % 32 != 16)
        return;
    f = tp_mtof(tp_chord_at(s)->root + 24);
    v = tv_new(l, off);                       /* struck, then ringing: two partials beating against each other */
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 2.71f, 0.0f, 0.35f);
    tv_osc(v, W_SINE, f * 2.71f, 7.0f, 0.35f);
    tv_amp(v, 0.85f);
    tv_lfo(v, LFO_AMP, 1.7f, 0.15f, 0);
    tv_env_ad(v, 0.01f, 0.17f, 6.0f);
    tv_len(v, 6.2f);
}

static void low_koto(int l, int s, int off)
{
    int p = s % 16;
    float f;
    tp_voice_t *v;
    if (p != 0 && p != 6 && p != 12)
        return;
    f = tp_mtof(tp_chord_at(s)->root + (p == 6 ? 19 : 12));
    v = tv_new(l, off);
    tv_osc(v, W_TRI, f, 0.0f, 1.0f);
    tv_osc(v, W_SAW, f, 0.0f, 0.3f);
    tv_filter(v, F_LP, 1400.0f, 1.0f);
    tv_filter_ramp(v, 1, 300.0f, 0.5f);
    tv_env_ad(v, 0.002f, p ? 0.22f : 0.32f, 1.3f);
    tv_len(v, 1.4f);
}

/* ---- Garden ---- */
static void bamboo_fountain(int l, int s, int off)
{
    tp_voice_t *v;
    if (s % 64 == 0 || (s % 64 == 32 && tp_rand() < 0.5f)) {   /* the bamboo tips over: a hollow knock */
        v = tv_new(l, off);
        tv_osc(v, W_SINE, 340.0f, 0.0f, 1.0f);
        tv_osc(v, W_SINE, 870.0f, 0.0f, 0.4f);
        tv_pitch_ramp(v, 300.0f, 0.1f);
        tv_env_ad(v, 0.001f, 0.32f, 0.35f);
        tv_len(v, 0.4f);
        tp_burst(l, off, F_BP, 1200.0f, 2.0f, 0.001f, 0.12f, 0.06f, 0.1f);
    } else if (tp_rand() < 0.22f) {           /* and the water running into it */
        int at = off_after(off, tp_rand() * tp_sps());
        v = tv_new(l, at);
        tv_osc(v, W_SINE, 900.0f + tp_rand() * 900.0f, 0.0f, 1.0f);
        tv_pitch_ramp(v, 1900.0f + tp_rand() * 600.0f, 0.04f);
        tv_env_ad(v, 0.002f, 0.025f, 0.05f);
        tv_len(v, 0.07f);
    }
}

static void rain_leaves_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);
    tv_persist(v);
    tv_noise(v, 0, 1.0f);
    tv_filter(v, F_BP, 4200.0f, 0.9f);
    tv_env_const(v, 0.07f);
    tv_lfo(v, LFO_GAIN, 0.07f, 0.03f, 0);
}
static void rain_leaves(int l, int s, int off)
{
    int n = (int)(tp_rand() * 3.0f), i;
    for (i = 0; i < n; i++) {                 /* drops on the leaves */
        int at = off_after(off, tp_rand() * tp_sps());
        tp_burst(l, at, F_BP, 2500.0f + tp_rand() * 4000.0f, 6.0f, 0.001f, 0.04f + tp_rand() * 0.05f, 0.02f, 0.05f);
    }
}

static void wind_chimes(int l, int s, int off)
{
    int n, i;
    if (s % 4 || tp_rand() > 0.22f)
        return;
    n = 1 + (int)(tp_rand() * 4.0f);
    for (i = 0; i < n; i++) {
        float f = tp_mtof(P8[(int)(tp_rand() * 8.0f)] + 12);
        tp_voice_t *v = tv_new(l, off_after(off, tp_rand() * tp_sps() * 3.0f));
        tv_osc(v, W_SINE, f, 0.0f, 1.0f);
        tv_osc(v, W_SINE, f * 2.76f, 0.0f, 0.2f);
        tv_env_ad(v, 0.001f, 0.055f, 2.0f);
        tv_len(v, 2.1f);
    }
}

static void cicadas_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);           /* a shimmering hiss, trembling fast, rising and falling slowly */
    tv_persist(v);
    tv_noise(v, 0, 1.0f);
    tv_filter(v, F_BP, 5800.0f, 5.0f);
    tv_env_const(v, 0.22f);
    tv_amp(v, 0.6f);
    tv_lfo(v, LFO_AMP, 28.0f, 0.4f, 0);
    tv_lfo(v, LFO_GAIN, 0.06f, 0.18f, 0);
}

/* ---- Songs ---- */
static void koto(int l, int s, int off)
{
    int m = tp_kal[s % 64];
    float f;
    tp_voice_t *v;
    if (!m)
        return;
    f = tp_mtof(m);
    v = tv_new(l, off);                       /* a plucked silk string: bright, then soft, a little bend down */
    tv_osc(v, W_SAW, f * 1.01f, 0.0f, 0.6f);
    tv_osc(v, W_TRI, f * 1.01f, 0.0f, 1.0f);
    tv_pitch_ramp(v, f, 0.08f);
    tv_filter(v, F_LP, 4000.0f, 2.0f);
    tv_filter_ramp(v, 1, 900.0f, 0.3f);
    tv_env_ad(v, 0.002f, 0.07f, 1.3f);
    tv_len(v, 1.4f);
}

static void shakuhachi(int l, int s, int off)
{
    float len = tp_sps() * 8.0f, f;
    tp_voice_t *v;
    if (s % 32 != 4 || tp_rand() > 0.6f)
        return;
    f = tp_mtof(P8[(int)(tp_rand() * 5.0f)]);
    v = tv_new(l, off);                       /* breath first, then the note bending up into place */
    tv_osc(v, W_SINE, f * 0.95f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 1.9f, 0.0f, 0.1f);
    tv_pitch_ramp(v, f, 0.5f);
    tv_lfo(v, LFO_DET, 4.0f, 11.0f, 3);
    tv_env_asr(v, 0.5f, 0.1f, len, 1.5f);
    tv_len(v, len + 2.0f);
    tp_burst(l, off, F_BP, 1600.0f, 0.7f, 0.2f, 0.045f, 0.7f, 1.0f);
}

static void temple_bell(int l, int s, int off)
{
    float f;
    tp_voice_t *v;
    if (s % 64 != 48 || tp_rand() > 0.75f)
        return;
    f = tp_mtof(tp_chord_at(s)->root + 24);
    v = tv_new(l, off);
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 0.5f, 0.0f, 0.5f);
    tv_fm(v, 1.4f, f * 1.2f, 2.5f);
    tv_env_ad(v, 0.003f, 0.11f, 7.0f);
    tv_len(v, 7.2f);
}

static void water_drops(int l, int s, int off)
{
    float f;
    tp_voice_t *v;
    if (s % 2 || tp_rand() > 0.3f)
        return;
    f = tp_mtof(P8[(int)(tp_rand() * 8.0f)] + 12);
    v = tv_new(l, off);                       /* a drop in a stone basin: a ringing ping, sliding up a touch */
    tv_osc(v, W_SINE, f * 0.97f, 0.0f, 1.0f);
    tv_pitch_ramp(v, f, 0.03f);
    tv_env_ad(v, 0.001f, 0.08f, 0.9f);
    tv_len(v, 1.0f);
}

/* ---- Wood ---- */
static void taiko(int l, int s, int off)
{
    int p = s % 32;
    tp_voice_t *v;
    if (p != 0 && p != 22 && p != 24)
        return;
    v = tv_new(l, off);
    tv_osc(v, W_SINE, 95.0f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, 160.0f, 0.0f, 0.25f);
    tv_pitch_ramp(v, 55.0f, 0.25f);
    tv_env_ad(v, 0.003f, p ? 0.35f : 0.65f, 0.9f);
    tv_len(v, 1.0f);
    tp_burst(l, off, F_LP, 700.0f, 0.7f, 0.002f, 0.15f, 0.1f, 0.2f);
}

static void wood_clappers(int l, int s, int off)
{
    int p = s % 16;
    tp_voice_t *v;
    if (p != 4 && p != 12)
        return;
    v = tv_new(l, off);                       /* hyoshigi: two hard blocks */
    tv_osc(v, W_SINE, 2100.0f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, 3400.0f, 0.0f, 0.4f);
    tv_env_ad(v, 0.001f, p == 12 ? 0.14f : 0.1f, 0.06f);
    tv_len(v, 0.1f);
    tp_burst(l, off, F_BP, 2600.0f, 3.0f, 0.001f, 0.08f, 0.02f, 0.05f);
}

static void small_bell(int l, int s, int off)
{
    tp_voice_t *v;
    if (s % 16 != 10 || tp_rand() > 0.55f)
        return;
    v = tv_new(l, off);
    tv_osc(v, W_SINE, 1760.0f, 0.0f, 1.0f);
    tv_fm(v, 2.4f, 900.0f, 0.3f);
    tv_env_ad(v, 0.001f, 0.045f, 1.6f);
    tv_len(v, 1.7f);
}

static void sand_rake(int l, int s, int off)
{
    tp_voice_t *v;
    if (s % 8 != 0)
        return;
    v = tv_new(l, off);                       /* a long brush through gravel, rising and falling */
    tv_noise(v, 0, 1.0f);
    tv_filter(v, F_BP, 1800.0f, 1.5f);
    tv_filter_ramp(v, 0, 3200.0f, tp_sps() * 3.0f);
    tv_env_asr(v, tp_sps() * 2.5f, s % 16 ? 0.05f : 0.07f, 0.0f, tp_sps() * 2.5f);
    tv_len(v, tp_sps() * 5.2f);
}

static const tp_loop_t LOOPS[TP_NLOOP] = {
    /* name             fam  pan     rev    dly    fade  start              sched */
    { "Temple drone",   0,  0.0f,  0.35f, 0.0f,  4.0f,  temple_drone,      0 },
    { "Sho",            0, -0.1f,  0.55f, 0.0f,  1.0f,  0,                 sho },
    { "Singing bowl",   0,  0.2f,  0.6f,  0.2f,  1.0f,  0,                 singing_bowl },
    { "Low koto",       0, -0.15f, 0.35f, 0.15f, 0.5f,  0,                 low_koto },
    { "Bamboo fountain",1, -0.3f,  0.4f,  0.15f, 0.5f,  0,                 bamboo_fountain },
    { "Rain on leaves", 1,  0.0f,  0.3f,  0.0f,  3.0f,  rain_leaves_start, rain_leaves },
    { "Wind chimes",    1,  0.35f, 0.6f,  0.25f, 0.5f,  0,                 wind_chimes },
    { "Cicadas",        1,  0.15f, 0.25f, 0.0f,  3.0f,  cicadas_start,     0 },
    { "Koto",           2, -0.2f,  0.45f, 0.3f,  0.3f,  0,                 koto },
    { "Shakuhachi",     2,  0.2f,  0.7f,  0.35f, 0.5f,  0,                 shakuhachi },
    { "Temple bell",    2,  0.0f,  0.75f, 0.2f,  0.5f,  0,                 temple_bell },
    { "Water drops",    2,  0.3f,  0.55f, 0.45f, 0.3f,  0,                 water_drops },
    { "Taiko",          3,  0.0f,  0.45f, 0.0f,  0.05f, 0,                 taiko },
    { "Wood clappers",  3, -0.25f, 0.45f, 0.2f,  0.05f, 0,                 wood_clappers },
    { "Small bell",     3,  0.3f,  0.55f, 0.3f,  0.05f, 0,                 small_bell },
    { "Sand rake",      3, -0.1f,  0.3f,  0.0f,  0.05f, 0,                 sand_rake },
};

const tp_bank_t TP_ZEN = { "Zen Garden", LOOPS, CHORDS, P8, 8, 73, 0.2f, { "Stillness", "Garden", "Songs", "Wood" },
                           TP_STYLE_FOLK, 0 };
