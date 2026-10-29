/* Tidepool on the FM-1: Bank 6, Aurora. Written for the FM-1 (no web app original): a polar night under the
 * northern lights, in D major. Glassy pads and bowed glass over a low drone; ice, wind and whale song; bells, celesta
 * and a high arp; a slow heartbeat of drums, sleigh bells and ticks. */
#include "tp_voice.h"
#include "tp_math.h"

#define SR TP_SR
#define off_after(off, sec) ((off) + (int)((sec) * SR))

static const tp_chord_t CHORDS[4] = {
    { "Dmaj9", 38, { 50, 54, 57, 61, 64 } },
    { "Bm9", 35, { 50, 54, 57, 59, 61 } },
    { "Gmaj7", 43, { 50, 54, 55, 59, 62 } },
    { "Aadd9", 33, { 52, 57, 59, 61, 64 } },
};
static const uint8_t P6[8] = { 74, 76, 78, 81, 83, 86, 88, 90 };   /* D major pentatonic, high */

/* ---- Skies ---- */
static void polar_drone(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);
    tv_persist(v);
    tv_osc(v, W_SINE, tp_mtof(26), 0.0f, 1.0f);
    tv_osc(v, W_SINE, tp_mtof(33), 0.0f, 0.45f);
    tv_osc(v, W_TRI, tp_mtof(38), -4.0f, 0.25f);
    tv_osc(v, W_TRI, tp_mtof(38), 4.0f, 0.25f);
    tv_filter(v, F_LP, 420.0f, 1.0f);
    tv_env_const(v, 0.17f);
    tv_lfo(v, LFO_FC, 0.04f, 160.0f, 0);
}

static void aurora_pad(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float bar = 16.0f * tp_sps();
    int h;
    if (s % 16)
        return;
    for (h = 0; h < 2; h++) {                /* eight saws, two voices of four, a slow sweep up and back */
        tp_voice_t *v = tv_new(l, off);
        int k;
        for (k = h * 4; k < h * 4 + 4; k++)
            tv_osc(v, W_SAW, tp_mtof(c->notes[1 + k / 2] + (h ? 12 : 0)), k % 2 ? 9.0f : -9.0f, 1.0f);
        tv_filter(v, F_LP, 380.0f, 3.0f);
        tv_filter_ramp(v, 0, 2200.0f, bar * 0.5f);
        tv_filter_ramp(v, 0, 500.0f, bar + 2.5f - bar * 0.5f);
        tv_env_asr(v, 2.2f, h ? 0.031f : 0.042f, bar - 2.2f, 2.5f);
        tv_len(v, bar + 2.7f);
    }
}

static void glass_bow(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float len = 32.0f * tp_sps(), f;
    tp_voice_t *v;
    if (s % 32 != 0)
        return;
    f = tp_mtof(c->notes[3] + 12);
    v = tv_new(l, off);                      /* bowed glass: a sine and its octave, a slow swell, a slight waver */
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 2.0f, 3.0f, 0.3f);
    tv_osc(v, W_SINE, f * 3.0f, -3.0f, 0.08f);
    tv_lfo(v, LFO_DET, 3.1f, 6.0f, 7);
    tv_env_asr(v, 3.0f, 0.08f, len - 3.0f, 2.5f);
    tv_len(v, len + 2.7f);
}

static void low_swell(int l, int s, int off)
{
    float len = 32.0f * tp_sps(), f;
    tp_voice_t *v;
    if (s % 32)
        return;
    f = tp_mtof(tp_chord_at(s)->root + 12);
    v = tv_new(l, off);
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_osc(v, W_TRI, f * 2.0f, 0.0f, 0.2f);
    tv_env_asr(v, len * 0.45f, 0.3f, 0.0f, len * 0.6f);
    tv_len(v, len * 1.1f);
}

/* ---- Ice ---- */
static void blizzard_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);
    tv_persist(v);
    tv_noise(v, 1, 1.0f);
    tv_filter(v, F_BP, 700.0f, 1.5f);
    tv_env_const(v, 0.45f);
    tv_lfo(v, LFO_FC, 0.05f, 450.0f, 0);
    tv_lfo(v, LFO_GAIN, 0.09f, 0.3f, 0);
}

static void ice_cracks(int l, int s, int off)
{
    int at;
    tp_voice_t *v;
    if (tp_rand() > 0.05f)
        return;
    at = off_after(off, tp_rand() * tp_sps());   /* a crack: a sharp snap, a falling ping under it */
    tp_burst(l, at, F_HP, 3000.0f, 0.7f, 0.001f, 0.35f, 0.03f, 0.08f);
    v = tv_new(l, at);
    tv_osc(v, W_SINE, 1800.0f + tp_rand() * 1600.0f, 0.0f, 1.0f);
    tv_pitch_ramp(v, 500.0f, 0.25f);
    tv_env_ad(v, 0.001f, 0.12f, 0.25f);
    tv_len(v, 0.3f);
}

static void whale_song(int l, int s, int off)
{
    float f;
    tp_voice_t *v;
    if (s % 64 != 24 || tp_rand() > 0.6f)
        return;
    f = 260.0f + tp_rand() * 140.0f;        /* a long call, rising, then sinking further */
    v = tv_new(l, off);
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 2.0f, 0.0f, 0.15f);
    tv_pitch_ramp(v, f * 1.35f, 1.4f);
    tv_lfo(v, LFO_DET, 5.5f, 18.0f, 3);
    tv_filter(v, F_LP, 1200.0f, 0.7f);
    tv_env_asr(v, 0.6f, 0.08f, 1.0f, 1.6f);
    tv_len(v, 3.3f);
    v = tv_new(l, off_after(off, 1.6f));
    tv_osc(v, W_SINE, f * 1.3f, 0.0f, 1.0f);
    tv_pitch_ramp(v, f * 0.7f, 2.0f);
    tv_filter(v, F_LP, 1000.0f, 0.7f);
    tv_env_asr(v, 0.4f, 0.06f, 0.8f, 1.2f);
    tv_len(v, 2.5f);
}

static void snow_hiss_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);
    tv_persist(v);
    tv_noise(v, 0, 1.0f);
    tv_filter(v, F_BP, 7500.0f, 1.2f);
    tv_env_const(v, 0.05f);
    tv_lfo(v, LFO_GAIN, 0.13f, 0.03f, 0);
}

/* ---- Lights ---- */
static void ice_bells(int l, int s, int off)
{
    int m = tp_kal[s % 64];
    float f;
    tp_voice_t *v;
    if (!m)
        return;
    f = tp_mtof(m);
    v = tv_new(l, off);
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_fm(v, 3.5f, f * 1.6f, 0.4f);
    tv_env_ad(v, 0.002f, 0.06f, 2.2f);
    tv_len(v, 2.4f);
}

static void celesta(int l, int s, int off)
{
    float f;
    tp_voice_t *v;
    if (s % 4 != 2 || tp_rand() > 0.4f)
        return;
    f = tp_mtof(P6[(int)(tp_rand() * 8.0f)]);
    v = tv_new(l, off);
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 4.0f, 0.0f, 1.0f);
    tv_env_ad(v, 0.001f, 0.09f, 1.2f);
    tv_part_decay(v, 1, 0.032f, 0.12f, 0.09f, 1.2f);
    tv_len(v, 1.4f);
}

static void shimmer_arp(int l, int s, int off)
{
    static const int8_t pat[8] = { 0, 2, 4, 3, 1, 3, 4, 2 };
    float f;
    tp_voice_t *v;
    if (s % 2)
        return;
    f = tp_mtof(tp_chord_at(s)->notes[pat[(s / 2) % 8]] + 24);
    v = tv_new(l, off);
    tv_osc(v, W_TRI, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 2.0f, 0.0f, 0.25f);
    tv_filter(v, F_LP, 4000.0f, 0.7f);
    tv_env_ad(v, 0.003f, 0.05f, 0.7f);
    tv_len(v, 0.8f);
}

static void spirit_flute(int l, int s, int off)
{
    float len = tp_sps() * 7.0f;
    int m;
    tp_voice_t *v;
    if (s % 16 != 8 || tp_rand() > 0.55f)
        return;
    m = P6[(int)(tp_rand() * 5.0f)] - 12;
    v = tv_new(l, off);
    tv_osc(v, W_SINE, tp_mtof(m), 0.0f, 1.0f);
    tv_osc(v, W_SINE, tp_mtof(m) * 2.0f, 0.0f, 0.08f);
    tv_lfo(v, LFO_DET, 4.4f, 7.0f, 3);
    tv_env_asr(v, 0.3f, 0.09f, len, 1.2f);
    tv_len(v, len + 1.6f);
    tp_burst(l, off, F_BP, 2600.0f, 1.0f, 0.1f, 0.015f, 0.4f, 0.6f);
}

/* ---- Pulse ---- */
static void heart_drum(int l, int s, int off)
{
    int p = s % 16;
    tp_voice_t *v;
    if (p != 0 && p != 3)
        return;
    v = tv_new(l, off);                      /* lub-dub, the second softer */
    tv_osc(v, W_SINE, p ? 75.0f : 68.0f, 0.0f, 1.0f);
    tv_pitch_ramp(v, 42.0f, 0.18f);
    tv_env_ad(v, 0.004f, p ? 0.5f : 0.85f, 0.35f);
    tv_len(v, 0.45f);
}

static void sleigh_bells(int l, int s, int off)
{
    int p = s % 8;
    if (p != 2 && p != 6)
        return;
    tp_burst(l, off, F_HP, 8500.0f, 0.7f, 0.004f, p == 6 ? 0.07f : 0.045f, 0.12f, 0.2f);
    tp_burst(l, off_after(off, 0.03f), F_BP, 11000.0f, 4.0f, 0.002f, 0.03f, 0.1f, 0.15f);
}

static void ice_tick(int l, int s, int off)
{
    tp_voice_t *v;
    if (s % 4 != 1 || tp_rand() > 0.7f)
        return;
    v = tv_new(l, off);
    tv_osc(v, W_SINE, 3200.0f, 0.0f, 1.0f);
    tv_fm(v, 1.41f, 2400.0f, 0.03f);
    tv_env_ad(v, 0.001f, 0.06f, 0.06f);
    tv_len(v, 0.1f);
}

static void deep_boom(int l, int s, int off)
{
    tp_voice_t *v;
    if (s % 32 != 0)
        return;
    v = tv_new(l, off);
    tv_osc(v, W_SINE, 62.0f, 0.0f, 1.0f);
    tv_pitch_ramp(v, 34.0f, 0.6f);
    tv_env_ad(v, 0.01f, 0.7f, 1.8f);
    tv_len(v, 2.0f);
    tp_burst(l, off, F_LP, 300.0f, 0.7f, 0.005f, 0.2f, 0.4f, 0.6f);
}

static const tp_loop_t LOOPS[TP_NLOOP] = {
    /* name             fam  pan     rev    dly    fade  start            sched */
    { "Polar drone",    0,  0.0f,  0.35f, 0.0f,  4.0f,  polar_drone,     0 },
    { "Aurora pad",     0, -0.15f, 0.6f,  0.0f,  1.0f,  0,               aurora_pad },
    { "Glass bow",      0,  0.2f,  0.65f, 0.2f,  1.0f,  0,               glass_bow },
    { "Low swell",      0,  0.0f,  0.3f,  0.0f,  1.0f,  0,               low_swell },
    { "Blizzard",       1,  0.0f,  0.25f, 0.0f,  3.0f,  blizzard_start,  0 },
    { "Ice cracks",     1,  0.3f,  0.55f, 0.2f,  0.5f,  0,               ice_cracks },
    { "Whale song",     1, -0.25f, 0.8f,  0.35f, 0.5f,  0,               whale_song },
    { "Snow hiss",      1,  0.0f,  0.3f,  0.0f,  3.0f,  snow_hiss_start, 0 },
    { "Ice bells",      2, -0.25f, 0.55f, 0.4f,  0.3f,  0,               ice_bells },
    { "Celesta",        2,  0.3f,  0.5f,  0.45f, 0.3f,  0,               celesta },
    { "Shimmer arp",    2,  0.1f,  0.45f, 0.35f, 0.3f,  0,               shimmer_arp },
    { "Spirit flute",   2, -0.1f,  0.7f,  0.3f,  0.5f,  0,               spirit_flute },
    { "Heart drum",     3,  0.0f,  0.35f, 0.0f,  0.05f, 0,               heart_drum },
    { "Sleigh bells",   3,  0.25f, 0.35f, 0.15f, 0.05f, 0,               sleigh_bells },
    { "Ice tick",       3, -0.3f,  0.4f,  0.3f,  0.05f, 0,               ice_tick },
    { "Deep boom",      3,  0.0f,  0.5f,  0.0f,  0.05f, 0,               deep_boom },
};

const tp_bank_t TP_AURORA = { "Aurora", LOOPS, CHORDS, P6, 8, 41, 0.22f, { "Skies", "Ice", "Lights", "Pulse" },
                              TP_STYLE_LUSH, 0 };
