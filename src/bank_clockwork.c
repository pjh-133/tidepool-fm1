/* Tidepool on the FM-1: Bank 9, Clockwork. Written for the FM-1 (no web app original): a clockmaker's workshop at
 * night, in G minor. A steam engine's hum, a reed organ, a musical saw and low gears; ticking gears, steam, a cuckoo
 * and an old radio; music box, celesta, toy piano and clarinet; the escapement's tick-tock, a ratchet, a hammer and a
 * pendulum. */
#include "tp_voice.h"
#include "tp_math.h"

#define SR TP_SR
#define off_after(off, sec) ((off) + (int)((sec) * SR))

static const tp_chord_t CHORDS[4] = {
    { "Gm", 43, { 50, 55, 58, 62, 67 } },
    { "Ebmaj7", 39, { 51, 55, 58, 62, 67 } },
    { "Cm7", 36, { 51, 55, 58, 60, 63 } },
    { "Dm7", 38, { 50, 53, 57, 60, 65 } },
};
static const uint8_t P9[8] = { 67, 70, 72, 74, 77, 79, 82, 84 };   /* G minor pentatonic */

/* ---- Engine ---- */
static void steam_hum(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);
    tv_persist(v);
    tv_osc(v, W_SINE, tp_mtof(31), 0.0f, 1.0f);
    tv_osc(v, W_SAW, tp_mtof(43), -6.0f, 0.2f);
    tv_osc(v, W_SAW, tp_mtof(50), 6.0f, 0.12f);
    tv_filter(v, F_LP, 380.0f, 2.0f);
    tv_env_const(v, 0.22f);
    tv_lfo(v, LFO_FC, 0.06f, 140.0f, 0);
}

static void reed_organ(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float bar = 16.0f * tp_sps();
    int h, i;
    if (s % 16)
        return;
    for (h = 0; h < 2; h++) {                 /* pumped reeds: squares, a little breathy swell */
        tp_voice_t *v = tv_new(l, off);
        for (i = h * 2; i < h * 2 + 2; i++)
            tv_osc(v, W_SQR, tp_mtof(c->notes[i + 1]), i % 2 ? -3.0f : 3.0f, 1.0f);
        tv_filter(v, F_LP, 1300.0f, 0.7f);
        tv_amp(v, 0.85f);
        tv_lfo(v, LFO_AMP, 0.5f, 0.15f, 0);
        tv_env_asr(v, 0.6f, 0.035f, bar - 0.6f, 1.0f);
        tv_len(v, bar + 1.2f);
    }
}

static void musical_saw(int l, int s, int off)
{
    float len = 32.0f * tp_sps(), f;
    tp_voice_t *v;
    if (s % 32 != 8)
        return;
    f = tp_mtof(tp_chord_at(s)->notes[2 + (int)(tp_rand() * 3.0f)] + 12);
    v = tv_new(l, off);                       /* sliding up into the note, a wide, slow vibrato */
    tv_osc(v, W_SINE, f * 0.9f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 1.8f, 0.0f, 0.06f);
    tv_pitch_ramp(v, f, 0.6f);
    tv_lfo(v, LFO_DET, 5.6f, 22.0f, 3);
    tv_env_asr(v, 0.8f, 0.07f, len * 0.6f, 2.0f);
    tv_len(v, len * 0.6f + 3.0f);
}

static void low_gears(int l, int s, int off)
{
    int p = s % 8;
    float f;
    tp_voice_t *v;
    if (p != 0 && p != 6)
        return;
    f = tp_mtof(tp_chord_at(s)->root + (p ? 19 : 12));
    v = tv_new(l, off);                       /* oom-pa: a plucked low string, then its fifth */
    tv_osc(v, W_TRI, f, 0.0f, 1.0f);
    tv_osc(v, W_SQR, f, 0.0f, 0.15f);
    tv_filter(v, F_LP, 700.0f, 1.0f);
    tv_env_ad(v, 0.003f, p ? 0.25f : 0.4f, 0.5f);
    tv_len(v, 0.6f);
}

/* ---- Workshop ---- */
static void gears(int l, int s, int off)
{
    int n = 1 + (int)(tp_rand() * 2.5f), i;
    for (i = 0; i < n; i++) {                 /* small teeth catching, all over the room */
        int at = off_after(off, tp_rand() * tp_sps());
        tp_burst(l, at, F_BP, 3000.0f + tp_rand() * 5000.0f, 4.0f, 0.001f, 0.12f + tp_rand() * 0.12f, 0.012f, 0.03f);
    }
}

static void steam_hiss(int l, int s, int off)
{
    tp_voice_t *v;
    if (s % 16 != 12 || tp_rand() > 0.35f)
        return;
    v = tv_new(l, off);                       /* a valve letting go */
    tv_noise(v, 0, 1.0f);
    tv_filter(v, F_HP, 4000.0f, 0.7f);
    tv_filter_ramp(v, 0, 2000.0f, 1.5f);
    tv_env_asr(v, 0.05f, 0.08f, 0.4f, 1.2f);
    tv_len(v, 1.7f);
}

static void cuckoo(int l, int s, int off)
{
    int i;
    if (s % 64 != 32 || tp_rand() > 0.5f)
        return;
    for (i = 0; i < 2; i++) {                 /* cu-ckoo: a major third down */
        tp_voice_t *v = tv_new(l, off_after(off, i * 0.35f));
        tv_osc(v, W_SINE, tp_mtof(i ? 74 : 78), 0.0f, 1.0f);
        tv_osc(v, W_TRI, tp_mtof(i ? 74 : 78), 0.0f, 0.3f);
        tv_filter(v, F_LP, 2000.0f, 0.7f);
        tv_env_ad(v, 0.01f, 0.14f, 0.28f);
        tv_len(v, 0.32f);
    }
}

static void old_radio_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);           /* a radio between stations: a narrow band of hiss, drifting */
    tv_persist(v);
    tv_noise(v, 0, 1.0f);
    tv_filter(v, F_BP, 1400.0f, 3.0f);
    tv_env_const(v, 0.14f);
    tv_lfo(v, LFO_FC, 0.09f, 600.0f, 0);
    v = tv_new(l, off);                       /* and, far away, a carrier's whine */
    tv_persist(v);
    tv_osc(v, W_SINE, 1320.0f, 0.0f, 1.0f);
    tv_env_const(v, 0.006f);
    tv_lfo(v, LFO_DET, 0.2f, 60.0f, 1);
}

/* ---- Mechanisms ---- */
static void music_box(int l, int s, int off)
{
    int m = tp_kal[s % 64];
    float f;
    tp_voice_t *v;
    if (!m)
        return;
    f = tp_mtof(m + 12);
    v = tv_new(l, off);                       /* a comb's tooth: a sine and a faint high partial */
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 5.4f, 0.0f, 1.0f);
    tv_env_ad(v, 0.001f, 0.06f, 1.5f);
    tv_part_decay(v, 1, 0.01f, 0.05f, 0.06f, 1.5f);
    tv_len(v, 1.6f);
}

static void celesta(int l, int s, int off)
{
    float f;
    tp_voice_t *v;
    if (s % 4 != 2 || tp_rand() > 0.35f)
        return;
    f = tp_mtof(P9[(int)(tp_rand() * 8.0f)]);
    v = tv_new(l, off);
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_fm(v, 4.0f, f * 0.8f, 0.2f);
    tv_env_ad(v, 0.001f, 0.09f, 1.1f);
    tv_len(v, 1.2f);
}

static void toy_piano(int l, int s, int off)
{
    static const int8_t pat[16] = { 0, -1, 2, -1, 4, -1, 2, -1, 1, -1, 3, -1, 4, 3, 2, -1 };
    int idx = pat[s % 16];
    float f;
    tp_voice_t *v;
    if (idx < 0 || tp_rand() > 0.8f)
        return;
    f = tp_mtof(tp_chord_at(s)->notes[idx] + 12);
    v = tv_new(l, off);                       /* a struck metal rod: a slightly sour overtone */
    tv_osc(v, W_TRI, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 3.9f, 0.0f, 0.5f);
    tv_env_ad(v, 0.001f, 0.065f, 0.8f);
    tv_part_decay(v, 1, 0.04f, 0.15f, 0.065f, 0.8f);
    tv_len(v, 0.9f);
}

static void clarinet(int l, int s, int off)
{
    float len = tp_sps() * 6.0f, f;
    tp_voice_t *v;
    if (s % 16 != 0 || tp_rand() > 0.5f)
        return;
    f = tp_mtof(P9[(int)(tp_rand() * 6.0f)] - 12);
    v = tv_new(l, off);                       /* a hollow square, softened, a gentle vibrato */
    tv_osc(v, W_SQR, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f, 0.0f, 0.5f);
    tv_lfo(v, LFO_DET, 5.0f, 6.0f, 3);
    tv_filter(v, F_LP, 1500.0f, 0.7f);
    tv_env_asr(v, 0.12f, 0.035f, len, 0.6f);
    tv_len(v, len + 0.8f);
}

/* ---- Escapement ---- */
static void tick_tock(int l, int s, int off)
{
    tp_voice_t *v;
    if (s % 4)
        return;
    v = tv_new(l, off);                       /* tick high, tock low, on every beat */
    tv_osc(v, W_SINE, s % 8 ? 1300.0f : 1750.0f, 0.0f, 1.0f);
    tv_env_ad(v, 0.001f, 0.18f, 0.035f);
    tv_len(v, 0.06f);
    tp_burst(l, off, F_BP, s % 8 ? 2200.0f : 3000.0f, 4.0f, 0.001f, 0.06f, 0.015f, 0.04f);
}

static void ratchet(int l, int s, int off)
{
    int p = s % 32, i;
    if (p != 28)
        return;
    for (i = 0; i < 8; i++)                   /* a wound spring: a quick run of clicks */
        tp_burst(l, off_after(off, i * tp_sps() * 0.5f), F_BP, 4500.0f, 2.0f, 0.001f, 0.12f + i * 0.02f, 0.012f,
                 0.03f);
}

static void hammer(int l, int s, int off)
{
    tp_voice_t *v;
    if (s % 16 != 8 || tp_rand() > 0.7f)
        return;
    v = tv_new(l, off);                       /* a small hammer on an anvil */
    tv_osc(v, W_SINE, 1150.0f, 0.0f, 1.0f);
    tv_fm(v, 2.7f, 1500.0f, 0.15f);
    tv_env_ad(v, 0.001f, 0.08f, 0.5f);
    tv_len(v, 0.6f);
    tp_burst(l, off, F_HP, 3000.0f, 0.7f, 0.001f, 0.06f, 0.02f, 0.05f);
}

static void pendulum(int l, int s, int off)
{
    int p = s % 16;
    tp_voice_t *v;
    if (p != 0 && p != 8)
        return;
    v = tv_new(l, off);                       /* a soft wooden thud at each end of the swing */
    tv_osc(v, W_SINE, p ? 105.0f : 85.0f, 0.0f, 1.0f);
    tv_pitch_ramp(v, 55.0f, 0.12f);
    tv_env_ad(v, 0.003f, 0.8f, 0.25f);
    tv_len(v, 0.35f);
}

static const tp_loop_t LOOPS[TP_NLOOP] = {
    /* name             fam  pan     rev    dly    fade  start            sched */
    { "Steam hum",      0,  0.0f,  0.3f,  0.0f,  4.0f,  steam_hum,       0 },
    { "Reed organ",     0, -0.15f, 0.45f, 0.0f,  1.0f,  0,               reed_organ },
    { "Musical saw",    0,  0.2f,  0.6f,  0.25f, 1.0f,  0,               musical_saw },
    { "Low gears",      0,  0.0f,  0.25f, 0.0f,  0.5f,  0,               low_gears },
    { "Gears",          1,  0.2f,  0.3f,  0.0f,  1.0f,  0,               gears },
    { "Steam hiss",     1, -0.3f,  0.4f,  0.0f,  0.5f,  0,               steam_hiss },
    { "Cuckoo",         1,  0.35f, 0.5f,  0.2f,  0.5f,  0,               cuckoo },
    { "Old radio",      1, -0.2f,  0.3f,  0.0f,  3.0f,  old_radio_start, 0 },
    { "Music box",      2,  0.25f, 0.5f,  0.4f,  0.3f,  0,               music_box },
    { "Celesta",        2, -0.25f, 0.5f,  0.4f,  0.3f,  0,               celesta },
    { "Toy piano",      2,  0.1f,  0.4f,  0.3f,  0.3f,  0,               toy_piano },
    { "Clarinet",       2, -0.1f,  0.55f, 0.3f,  0.5f,  0,               clarinet },
    { "Tick tock",      3,  0.0f,  0.25f, 0.0f,  0.05f, 0,               tick_tock },
    { "Ratchet",        3,  0.3f,  0.35f, 0.2f,  0.05f, 0,               ratchet },
    { "Hammer",         3, -0.25f, 0.45f, 0.3f,  0.05f, 0,               hammer },
    { "Pendulum",       3,  0.0f,  0.3f,  0.0f,  0.05f, 0,               pendulum },
};

const tp_bank_t TP_CLOCKWORK = { "Clockwork", LOOPS, CHORDS, P9, 8, 89, 0.24f,
                                 { "Engine", "Workshop", "Mechanisms", "Escapement" }, TP_STYLE_LUSH, 0 };
