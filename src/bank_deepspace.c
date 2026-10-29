/* Tidepool on the FM-1: Bank 2, Deep Space. A line-by-line port of BANK2_LOOPS in public/index.html.
 * A chord's oscillators that share a filter and an envelope in the web app share a voice here; Nebula pad's ten
 * saws are two voices of five, each with the same filter sweep. Hull creak's linear pitch glide is exponential here
 * (over at most 15 % the two are the same). */
#include "tp_voice.h"
#include "tp_math.h"

#define SR TP_SR
#define off_after(off, sec) ((off) + (int)((sec) * SR))

static const tp_chord_t CHORDS[4] = {
    { "Em9", 40, { 52, 55, 59, 62, 66 } },
    { "Cmaj7", 36, { 48, 52, 55, 59, 64 } },
    { "Am9", 33, { 57, 60, 64, 67, 71 } },
    { "Dsus2", 38, { 50, 52, 57, 62, 64 } },
};
static const uint8_t P2[9] = { 64, 67, 69, 71, 74, 76, 79, 81, 83 };

/* ---- Drones ---- */
static void event_horizon(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);
    tv_persist(v);
    tv_osc(v, W_SINE, tp_mtof(28), 0.0f, 1.0f);
    tv_osc(v, W_SAW, tp_mtof(40), -5.0f, 0.28f);
    tv_osc(v, W_SAW, tp_mtof(40), 6.0f, 0.28f);
    tv_osc(v, W_TRI, tp_mtof(47), 0.0f, 0.3f);
    tv_filter(v, F_LP, 360.0f, 1.5f);
    tv_env_const(v, 0.18f);
    tv_lfo(v, LFO_FC, 0.035f, 200.0f, 0);
}

static void nebula_pad(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float bar = 16.0f * tp_sps();
    int h;
    if (s % 16)
        return;
    for (h = 0; h < 2; h++) {                /* ten detuned saws: two voices of five, the same sweep */
        tp_voice_t *v = tv_new(l, off);
        int k;
        for (k = h * 5; k < h * 5 + 5; k++)
            tv_osc(v, W_SAW, tp_mtof(c->notes[k / 2]), k % 2 ? 8.0f : -8.0f, 1.0f);
        tv_filter(v, F_LP, 300.0f, 2.0f);
        tv_filter_ramp(v, 0, 1500.0f, bar * 0.6f);
        tv_filter_ramp(v, 0, 400.0f, bar + 2.0f - bar * 0.6f);
        tv_env_asr(v, 2.0f, 0.03f, bar - 2.0f, 2.5f);
        tv_len(v, bar + 2.7f);
    }
}

static void ion_choir(int l, int s, int off)
{
    static const uint8_t pick[3] = { 0, 2, 4 };
    const tp_chord_t *c = tp_chord_at(s);
    float len = 32.0f * tp_sps();
    tp_voice_t *v;
    int i;
    if (s % 32)
        return;
    v = tv_new(l, off);
    for (i = 0; i < 3; i++) {
        tv_osc(v, W_SAW, tp_mtof(c->notes[pick[i]] + 12), 0.0f, 1.0f);
        tv_lfo(v, LFO_DET, 4.2f + tp_rand() * 0.5f, 7.0f, 1 << i);
    }
    tv_filter(v, F_BP, 850.0f, 2.5f);
    tv_env_asr(v, 2.5f, 0.09f, len - 2.5f, 3.0f);
    tv_len(v, len + 3.2f);
}

static void sub_swell(int l, int s, int off)
{
    tp_voice_t *v;
    if (s % 32)
        return;
    v = tv_new(l, off);
    tv_osc(v, W_SINE, tp_mtof(tp_chord_at(s)->root), 0.0f, 1.0f);
    tv_env_asr(v, 3.2f, 0.4f, 0.4f, 3.2f);
    tv_len(v, 7.0f);
}

/* ---- Atmosphere ---- */
static void radio_static_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);
    tv_persist(v);
    tv_noise(v, 0, 1.0f);
    tv_filter(v, F_BP, 3200.0f, 0.8f);
    tv_env_const(v, 0.035f);
    tv_lfo(v, LFO_GAIN, 0.31f, 0.02f, 0);
    tv_lfo(v, LFO_FC, 0.07f, 900.0f, 0);
}
static void radio_static(int l, int s, int off)
{
    if (tp_rand() < 0.12f) {
        int at = off_after(off, tp_rand() * tp_sps());
        tp_burst(l, at, F_BP, 600.0f + tp_rand() * 3000.0f, 9.0f, 0.001f, 0.08f, 0.18f, 0.3f);
    }
}

static void solar_wind_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);
    tv_persist(v);
    tv_noise(v, 1, 1.0f);
    tv_filter(v, F_BP, 320.0f, 3.0f);
    tv_env_const(v, 0.5f);
    tv_lfo(v, LFO_FC, 0.03f, 220.0f, 0);
    tv_lfo(v, LFO_GAIN, 0.08f, 0.25f, 0);
}

static void sonar(int l, int s, int off)
{
    tp_voice_t *v;
    if (s % 32 != 8)
        return;
    v = tv_new(l, off);
    tv_osc(v, W_SINE, tp_mtof(tp_chord_at(s)->notes[4] + 12), 0.0f, 1.0f);
    tv_env_ad(v, 0.005f, 0.14f, 1.6f);
    tv_len(v, 1.8f);
}

static void hull_creak(int l, int s, int off)
{
    float f, to, bp;
    tp_voice_t *v;
    if (tp_rand() > 0.05f)
        return;
    f = 55.0f + tp_rand() * 40.0f;
    bp = 180.0f + tp_rand() * 300.0f;
    to = f * (0.85f + tp_rand() * 0.3f);
    v = tv_new(l, off);
    tv_osc(v, W_SAW, f, 0.0f, 1.0f);
    tv_pitch_ramp(v, to, 1.4f);
    tv_filter(v, F_BP, bp, 14.0f);
    tv_env_asr(v, 0.3f, 0.12f, 0.5f, 0.7f);
    tv_len(v, 1.6f);
}

/* ---- Melodies ---- */
static void star_bells(int l, int s, int off)
{
    float f;
    tp_voice_t *v;
    if (s % 4 || tp_rand() > 0.4f)
        return;
    f = tp_mtof(tp_chord_at(s)->notes[(int)(tp_rand() * 5.0f)] + 24);
    v = tv_new(l, off);
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_fm(v, 1.4f, f * 1.5f, 1.2f);
    tv_env_ad(v, 0.002f, 0.05f, 2.6f);
    tv_len(v, 2.8f);
}

static void lone_flute(int l, int s, int off)
{
    float len = tp_sps() * 6.0f;
    int m;
    tp_voice_t *v;
    if (s % 16 != 4 || tp_rand() > 0.65f)
        return;
    m = tp_chord_at(s)->notes[1 + (int)(tp_rand() * 4.0f)] + 12;
    v = tv_new(l, off);
    tv_osc(v, W_SINE, tp_mtof(m), 0.0f, 1.0f);
    tv_lfo(v, LFO_DET, 5.0f, 5.0f, 1);
    tv_env_asr(v, 0.3f, 0.08f, len, 0.9f);
    tv_len(v, len + 1.3f);
    tp_burst(l, off, F_BP, 2400.0f, 1.2f, 0.15f, 0.02f, 0.5f, 0.8f);
}

static void falling_harp(int l, int s, int off)
{
    int p = s % 16;
    tp_voice_t *v;
    if (p > 8 || p % 2 || (s / 16) % 2)
        return;
    v = tv_new(l, off);
    tv_osc(v, W_TRI, tp_mtof(tp_chord_at(s)->notes[4 - p / 2] + 12), 0.0f, 1.0f);
    tv_filter(v, F_LP, 2600.0f, 0.7f);
    tv_env_ad(v, 0.003f, 0.1f, 1.3f);
    tv_len(v, 1.5f);
}

static void pulse_arp(int l, int s, int off)
{
    static const int8_t pat[8] = { 0, 3, 1, 4, 2, 3, 1, -1 };
    int idx;
    tp_voice_t *v;
    if (s % 2)
        return;
    idx = pat[(s / 2) % 8];
    if (idx < 0)
        return;
    v = tv_new(l, off);
    tv_osc(v, W_SQR, tp_mtof(tp_chord_at(s)->notes[idx]), 0.0f, 1.0f);
    tv_filter(v, F_LP, 2200.0f, 4.0f);
    tv_filter_ramp(v, 1, 350.0f, 0.22f);
    tv_env_ad(v, 0.002f, 0.045f, 0.28f);
    tv_len(v, 0.4f);
}

/* ---- Pulse ---- */
static void heartbeat(int l, int s, int off)
{
    int p = s % 8;
    tp_voice_t *v;
    if (p != 0 && p != 2)
        return;
    v = tv_new(l, off);
    tv_osc(v, W_SINE, p ? 85.0f : 70.0f, 0.0f, 1.0f);
    tv_pitch_ramp(v, 38.0f, 0.16f);
    tv_env_ad(v, 0.004f, p ? 0.42f : 0.7f, 0.4f);
    tv_len(v, 0.5f);
}

static void tick_clock(int l, int s, int off)
{
    if (s % 2)
        return;
    tp_burst(l, off, F_BP, 4800.0f, 7.0f, 0.001f, s % 8 == 0 ? 0.13f : 0.06f, 0.014f, 0.05f);
}

static void deep_toms(int l, int s, int off)
{
    int p = s % 16;
    float f;
    tp_voice_t *v;
    if (p != 6 && p != 14 && !(p == 11 && tp_rand() < 0.4f))
        return;
    f = p == 14 ? 95.0f : 120.0f;
    v = tv_new(l, off);
    tv_osc(v, W_SINE, f * 1.5f, 0.0f, 1.0f);
    tv_pitch_ramp(v, f, 0.12f);
    tv_env_ad(v, 0.003f, 0.5f, 0.6f);
    tv_len(v, 0.7f);
}

static void metal_shimmer(int l, int s, int off)
{
    tp_voice_t *v;
    if (s % 32 != 0)
        return;
    tp_burst(l, off, F_BP, 4200.0f, 14.0f, 0.01f, 0.16f, 2.2f, 2.6f);
    v = tv_new(l, off);                      /* three partials, each its own level and decay */
    tv_osc(v, W_SINE, 1180.0f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, 1850.0f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, 2790.0f, 0.0f, 1.0f);
    tv_env_ad(v, 0.005f, 0.036f, 2.0f);
    tv_part_decay(v, 1, 0.018f, 1.6f, 0.036f, 2.0f);
    tv_part_decay(v, 2, 0.012f, 1.2f, 0.036f, 2.0f);
    tv_len(v, 2.2f);
}

static const tp_loop_t LOOPS[TP_NLOOP] = {
    /* name             fam  pan     rev    dly    fade  start               sched */
    { "Event horizon",  0,  0.0f,  0.5f,  0.0f,  4.0f,  event_horizon,      0 },
    { "Nebula pad",     0, -0.1f,  0.7f,  0.0f,  1.0f,  0,                  nebula_pad },
    { "Ion choir",      0, -0.2f,  0.85f, 0.0f,  1.5f,  0,                  ion_choir },
    { "Sub swell",      0,  0.0f,  0.2f,  0.0f,  1.0f,  0,                  sub_swell },
    { "Radio static",   1,  0.1f,  0.4f,  0.0f,  2.0f,  radio_static_start, radio_static },
    { "Solar wind",     1,  0.2f,  0.6f,  0.0f,  3.0f,  solar_wind_start,   0 },
    { "Sonar",          1, -0.25f, 0.7f,  0.6f,  0.2f,  0,                  sonar },
    { "Hull creak",     1, -0.1f,  0.5f,  0.0f,  0.5f,  0,                  hull_creak },
    { "Star bells",     2,  0.3f,  0.7f,  0.5f,  0.3f,  0,                  star_bells },
    { "Lone flute",     2, -0.25f, 0.7f,  0.35f, 0.5f,  0,                  lone_flute },
    { "Falling harp",   2,  0.15f, 0.6f,  0.4f,  0.3f,  0,                  falling_harp },
    { "Pulse arp",      2, -0.1f,  0.4f,  0.3f,  0.3f,  0,                  pulse_arp },
    { "Heartbeat",      3,  0.0f,  0.15f, 0.0f,  0.05f, 0,                  heartbeat },
    { "Tick clock",     3,  0.3f,  0.2f,  0.0f,  0.05f, 0,                  tick_clock },
    { "Deep toms",      3, -0.2f,  0.55f, 0.0f,  0.05f, 0,                  deep_toms },
    { "Metal shimmer",  3,  0.2f,  0.8f,  0.3f,  0.05f, 0,                  metal_shimmer },
};

const tp_bank_t TP_DEEPSPACE = { "Deep Space", LOOPS, CHORDS, P2, 9, 21, 0.18f,
                                 { "Drones", "Atmosphere", "Melodies", "Pulse" }, TP_STYLE_LUSH, 0 };
