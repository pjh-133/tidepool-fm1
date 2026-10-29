/* Tidepool on the FM-1: Bank 10, Night Café. Written for the FM-1 (no web app original): a late jazz café in the
 * rain, in Bb major, swung. Upright bass, Rhodes, organ and a soft pad; rain on the window, the murmur of the room,
 * cups and the espresso machine; muted trumpet, vibraphone, tenor sax and guitar; brushes, ride, a feathered kick and
 * a cross-stick. The offbeat eighths land late (swing), as a jazz player's do. */
#include "tp_voice.h"
#include "tp_math.h"

#define SR TP_SR
#define off_after(off, sec) ((off) + (int)((sec) * SR))
#define SWING 0.34f                              /* an offbeat eighth this many steps late */

static const tp_chord_t CHORDS[4] = {
    { "Bbmaj9", 34, { 50, 53, 57, 58, 60 } },
    { "Gm9", 43, { 50, 53, 57, 58, 62 } },
    { "Cm9", 36, { 51, 55, 58, 60, 62 } },
    { "F13", 41, { 51, 57, 60, 62, 65 } },
};
static const uint8_t P10[8] = { 70, 72, 74, 77, 79, 82, 84, 86 };   /* Bb major pentatonic */

static int swung(int s, int off)                 /* an offbeat eighth (step 2 of 4) late */
{
    return s % 4 == 2 ? off_after(off, tp_sps() * SWING) : off;
}

/* ---- Keys ---- */
static void upright_bass(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    int beat = (s % 16) / 4, m;
    float f;
    tp_voice_t *v;
    if (s % 4)
        return;
    switch (beat) {                           /* walking: root, third or fifth, fifth, then a step toward the next */
    case 0: m = c->root; break;
    case 1: m = c->notes[1] - 12; break;
    case 2: m = c->root + 7; break;
    default: {
        int next = tp_chord_at(s + 4)->root;
        m = next + (tp_rand() < 0.5f ? 1 : -1);
        break;
    }
    }
    while (m < 33)
        m += 12;
    while (m > 50)
        m -= 12;
    f = tp_mtof(m);
    v = tv_new(l, off);
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_osc(v, W_TRI, f * 2.0f, 0.0f, 0.25f);
    tv_filter(v, F_LP, 900.0f, 1.0f);
    tv_env_ad(v, 0.006f, 0.8f, 0.55f);
    tv_len(v, 0.65f);
}

static void rhodes(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    int p = s % 16, i;
    float pk;
    if (p != 0 && p != 6 && p != 10)          /* comping: the one, then a push before beat three, a late answer */
        return;
    if (p && tp_rand() > 0.7f)
        return;
    pk = p ? 0.037f : 0.05f;
    for (i = 0; i < 2; i++) {
        tp_voice_t *v = tv_new(l, swung(s, off));
        float f0 = tp_mtof(c->notes[i * 2 + 1]), f1 = tp_mtof(c->notes[i * 2 + 2]);
        tv_osc(v, W_SINE, f0, 0.0f, 1.0f);
        tv_osc(v, W_SINE, f1, 0.0f, 1.0f);
        tv_fm(v, 1.0f, f0 * 1.2f, 0.25f);
        tv_amp(v, 0.85f);
        tv_lfo(v, LFO_AMP, 4.5f, 0.15f, 0);
        tv_env_ad(v, 0.003f, pk, p ? 1.4f : 2.2f);
        tv_len(v, 2.4f);
    }
}

static void hammond(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float bar = 16.0f * tp_sps();
    tp_voice_t *v;
    int i;
    if (s % 16)
        return;
    v = tv_new(l, off);                       /* drawbars: the chord's notes and an octave, a slow rotary swirl */
    for (i = 0; i < 4; i++)
        tv_osc(v, W_SINE, tp_mtof(c->notes[i + 1]), (float)(i - 2), 1.0f);
    tv_osc(v, W_SINE, tp_mtof(c->notes[1] + 12), 0.0f, 0.5f);
    tv_amp(v, 0.8f);
    tv_lfo(v, LFO_AMP, 0.8f, 0.2f, 0);
    tv_env_asr(v, 0.25f, 0.045f, bar - 0.25f, 0.6f);
    tv_len(v, bar + 0.8f);
}

static void velvet_pad(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float len = 32.0f * tp_sps();
    tp_voice_t *v;
    int i;
    if (s % 32)
        return;
    v = tv_new(l, off);
    for (i = 0; i < 4; i++)
        tv_osc(v, W_SAW, tp_mtof(c->notes[i] + 12), i % 2 ? 7.0f : -7.0f, 1.0f);
    tv_filter(v, F_LP, 900.0f, 0.7f);
    tv_env_asr(v, 2.0f, 0.05f, len - 2.0f, 2.5f);
    tv_len(v, len + 2.7f);
}

/* ---- Café ---- */
static void rain_window_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);
    tv_persist(v);
    tv_noise(v, 1, 1.0f);
    tv_filter(v, F_BP, 2200.0f, 0.8f);
    tv_env_const(v, 0.25f);
    tv_lfo(v, LFO_GAIN, 0.05f, 0.08f, 0);
}
static void rain_window(int l, int s, int off)
{
    if (tp_rand() < 0.4f)                     /* a drop on the glass */
        tp_burst(l, off_after(off, tp_rand() * tp_sps()), F_BP, 3000.0f + tp_rand() * 2500.0f, 7.0f, 0.001f,
                 0.03f + tp_rand() * 0.04f, 0.02f, 0.05f);
}

static void murmur_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);           /* the room talking: noise through two moving vowel bands */
    tv_persist(v);
    tv_noise(v, 1, 1.0f);
    tv_filter(v, F_BP, 480.0f, 4.0f);
    tv_filter(v, F_BP, 1300.0f, 5.0f);
    tv_filter_parallel(v);
    tv_env_const(v, 0.55f);
    tv_lfo(v, LFO_FC, 2.3f, 120.0f, 0);
    tv_lfo(v, LFO_GAIN, 0.7f, 0.25f, 0);
}

static void clinking_cups(int l, int s, int off)
{
    int i, n;
    if (s % 8 || tp_rand() > 0.25f)
        return;
    n = 1 + (tp_rand() < 0.4f ? 1 : 0);
    for (i = 0; i < n; i++) {
        float f = 2400.0f + tp_rand() * 1800.0f;
        tp_voice_t *v = tv_new(l, off_after(off, tp_rand() * tp_sps() * 2.0f + i * 0.09f));
        tv_osc(v, W_SINE, f, 0.0f, 1.0f);
        tv_fm(v, 2.2f, f * 0.4f, 0.05f);
        tv_env_ad(v, 0.001f, 0.1f, 0.35f);
        tv_len(v, 0.4f);
    }
}

static void espresso(int l, int s, int off)
{
    tp_voice_t *v;
    if (s % 64 != 40 || tp_rand() > 0.6f)
        return;
    v = tv_new(l, off);                       /* steam, rising */
    tv_noise(v, 0, 1.0f);
    tv_filter(v, F_BP, 2500.0f, 1.5f);
    tv_filter_ramp(v, 0, 6000.0f, 2.0f);
    tv_env_asr(v, 0.3f, 0.07f, 1.5f, 0.6f);
    tv_len(v, 2.5f);
    v = tv_new(l, off);                       /* and milk bubbling under it */
    tv_noise(v, 1, 1.0f);
    tv_filter(v, F_BP, 350.0f, 3.0f);
    tv_amp(v, 0.6f);
    tv_lfo(v, LFO_AMP, 11.0f, 0.4f, 0);
    tv_env_asr(v, 0.3f, 0.2f, 1.5f, 0.6f);
    tv_len(v, 2.5f);
}

/* ---- Solos ---- */
static void muted_trumpet(int l, int s, int off)
{
    float len, f;
    tp_voice_t *v;
    if (s % 2 || tp_rand() > 0.16f || s % 64 >= 48)   /* phrases, a breath at the end of each cycle */
        return;
    f = tp_mtof(P10[(int)(tp_rand() * 6.0f)] - 12);
    len = tp_sps() * (tp_rand() < 0.5f ? 1.5f : 3.0f);
    v = tv_new(l, swung(s, off));             /* a harmon mute: a thin, nasal band of a saw */
    tv_osc(v, W_SAW, f, 0.0f, 1.0f);
    tv_lfo(v, LFO_DET, 5.5f, 5.0f, 1);
    tv_filter(v, F_BP, 1500.0f, 3.0f);
    tv_filter(v, F_HP, 600.0f, 0.7f);
    tv_env_asr(v, 0.03f, 0.18f, len, 0.15f);
    tv_len(v, len + 0.3f);
}

static void vibraphone(int l, int s, int off)
{
    int m = tp_kal[s % 64];
    float f;
    tp_voice_t *v;
    if (!m)
        return;
    f = tp_mtof(m);
    v = tv_new(l, swung(s, off));
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 4.0f, 0.0f, 1.0f);
    tv_amp(v, 0.8f);
    tv_lfo(v, LFO_AMP, 5.5f, 0.2f, 0);
    tv_env_ad(v, 0.002f, 0.06f, 2.2f);
    tv_part_decay(v, 1, 0.02f, 0.2f, 0.06f, 2.2f);
    tv_len(v, 2.4f);
}

static void tenor_sax(int l, int s, int off)
{
    float len = tp_sps() * 7.0f, f;
    tp_voice_t *v;
    if (s % 16 != 6 || tp_rand() > 0.5f)
        return;
    f = tp_mtof(tp_chord_at(s)->notes[1 + (int)(tp_rand() * 4.0f)]);
    v = tv_new(l, swung(s, off));             /* breathy, scooping up a little into the note */
    tv_osc(v, W_SAW, f * 0.97f, 0.0f, 1.0f);
    tv_osc(v, W_SQR, f * 0.97f, 0.0f, 0.3f);
    tv_pitch_ramp(v, f, 0.12f);
    tv_lfo(v, LFO_DET, 4.8f, 8.0f, 3);
    tv_filter(v, F_LP, 1300.0f, 1.5f);
    tv_env_asr(v, 0.12f, 0.045f, len, 0.6f);
    tv_len(v, len + 0.8f);
    tp_burst(l, swung(s, off), F_BP, 1800.0f, 1.0f, 0.06f, 0.02f, 0.3f, 0.5f);
}

static void jazz_guitar(int l, int s, int off)
{
    static const int8_t pat[8] = { 0, 2, 3, 1, 4, 3, 2, -1 };
    int idx;
    float f;
    tp_voice_t *v;
    if (s % 2)
        return;
    idx = pat[(s / 2) % 8];
    if (idx < 0 || tp_rand() > 0.75f)
        return;
    f = tp_mtof(tp_chord_at(s)->notes[idx] + 12);
    v = tv_new(l, swung(s, off));             /* a hollow-body: round and dark */
    tv_osc(v, W_TRI, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 2.0f, 0.0f, 0.2f);
    tv_filter(v, F_LP, 1700.0f, 0.7f);
    tv_env_ad(v, 0.003f, 0.085f, 0.6f);
    tv_len(v, 0.7f);
}

/* ---- Brushes ---- */
static void brush_swish(int l, int s, int off)
{
    tp_voice_t *v;
    if (s % 8 != 0)
        return;
    v = tv_new(l, off);                       /* a brush stirred round the snare, over two beats */
    tv_noise(v, 0, 1.0f);
    tv_filter(v, F_BP, 3500.0f, 1.0f);
    tv_env_asr(v, tp_sps() * 3.0f, 0.06f, 0.0f, tp_sps() * 4.0f);
    tv_len(v, tp_sps() * 7.2f);
}

static void ride(int l, int s, int off)
{
    int p = s % 8;
    tp_voice_t *v;
    if (p != 0 && p != 4 && p != 6)          /* ding, ding-a (the a swung) */
        return;
    v = tv_new(l, swung(s, off));
    tv_osc(v, W_SINE, 3300.0f, 0.0f, 1.0f);
    tv_fm(v, 1.47f, 4200.0f, 0.4f);
    tv_env_ad(v, 0.001f, p == 6 ? 0.025f : 0.04f, 0.9f);
    tv_len(v, 1.0f);
    tp_burst(l, swung(s, off), F_HP, 8000.0f, 0.7f, 0.001f, 0.03f, 0.15f, 0.2f);
}

static void kick_feather(int l, int s, int off)
{
    tp_voice_t *v;
    if (s % 4)
        return;
    v = tv_new(l, off);                       /* felt more than heard, on every beat */
    tv_osc(v, W_SINE, 70.0f, 0.0f, 1.0f);
    tv_pitch_ramp(v, 45.0f, 0.1f);
    tv_env_ad(v, 0.004f, s % 16 ? 0.5f : 0.7f, 0.25f);
    tv_len(v, 0.3f);
}

static void cross_stick(int l, int s, int off)
{
    int p = s % 16;
    tp_voice_t *v;
    if (p != 4 && p != 12 && !(p == 14 && tp_rand() < 0.2f))
        return;
    v = tv_new(l, swung(s, off));            /* the stick across the rim: a woody knock, and its crack */
    tv_osc(v, W_SINE, 1650.0f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, 560.0f, 0.0f, 0.5f);
    tv_env_ad(v, 0.001f, p == 14 ? 0.08f : 0.18f, 0.045f);
    tv_len(v, 0.08f);
    tp_burst(l, swung(s, off), F_BP, 2200.0f, 1.5f, 0.001f, p == 14 ? 0.2f : 0.45f, 0.03f, 0.06f);
}

static const tp_loop_t LOOPS[TP_NLOOP] = {
    /* name             fam  pan     rev    dly    fade  start              sched */
    { "Upright bass",   0,  0.0f,  0.2f,  0.0f,  0.5f,  0,                 upright_bass },
    { "Rhodes",         0, -0.2f,  0.4f,  0.15f, 1.0f,  0,                 rhodes },
    { "Organ",          0,  0.2f,  0.4f,  0.0f,  1.0f,  0,                 hammond },
    { "Velvet pad",     0,  0.0f,  0.55f, 0.0f,  1.0f,  0,                 velvet_pad },
    { "Rain on window", 1, -0.2f,  0.3f,  0.0f,  3.0f,  rain_window_start, rain_window },
    { "Murmur",         1,  0.15f, 0.35f, 0.0f,  3.0f,  murmur_start,      0 },
    { "Clinking cups",  1,  0.35f, 0.4f,  0.1f,  0.5f,  0,                 clinking_cups },
    { "Espresso",       1, -0.35f, 0.35f, 0.0f,  0.5f,  0,                 espresso },
    { "Muted trumpet",  2,  0.2f,  0.5f,  0.3f,  0.3f,  0,                 muted_trumpet },
    { "Vibraphone",     2, -0.25f, 0.5f,  0.35f, 0.3f,  0,                 vibraphone },
    { "Tenor sax",      2,  0.1f,  0.55f, 0.25f, 0.5f,  0,                 tenor_sax },
    { "Jazz guitar",    2, -0.1f,  0.35f, 0.2f,  0.3f,  0,                 jazz_guitar },
    { "Brush swish",    3,  0.1f,  0.3f,  0.0f,  0.05f, 0,                 brush_swish },
    { "Ride",           3,  0.3f,  0.35f, 0.0f,  0.05f, 0,                 ride },
    { "Soft kick",      3,  0.0f,  0.2f,  0.0f,  0.05f, 0,                 kick_feather },
    { "Side stick",     3, -0.2f,  0.35f, 0.15f, 0.05f, 0,                 cross_stick },
};

const tp_bank_t TP_CAFE = { "Night Cafe", LOOPS, CHORDS, P10, 8, 101, 0.2f, { "Keys", "Cafe", "Solos", "Brushes" },
                            TP_STYLE_LUSH, 0 };
