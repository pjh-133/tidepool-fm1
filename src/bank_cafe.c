/* Tidepool on the FM-1: Bank 10, Night Café. Written for the FM-1 (no web app original): lofi beats to study to,
 * in a café on a rainy night, in Bb major. Dusty Rhodes, a sub bass, a tape-warbled pad and lazy piano chords; vinyl
 * crackle, rain on the window, the murmur of the room and the espresso machine; a sleepy piano melody, a mellow
 * guitar, a breathy flute and a music box; a boom-bap kick, a dusty snare laid back behind the beat, swung hats and
 * finger snaps. Everything is a little muffled (low-passed) and a little seasick (tape wow: a slow wobble in pitch),
 * and the offbeat sixteenths land late, as a sampler's swing does. */
#include "tp_voice.h"
#include "tp_math.h"

#define SR TP_SR
#define off_after(off, sec) ((off) + (int)((sec) * SR))
#define SWING 0.3f                               /* an offbeat sixteenth this many steps late */
#define LAZY 0.1f                                /* the snare and snaps, this many steps behind the beat */

static const tp_chord_t CHORDS[4] = {
    { "Bbmaj9", 34, { 50, 53, 57, 58, 60 } },
    { "Ebmaj9", 39, { 51, 53, 55, 58, 62 } },
    { "Dm7", 38, { 50, 53, 57, 60, 62 } },
    { "Gm9", 43, { 50, 53, 57, 58, 62 } },
};
static const uint8_t P10[8] = { 70, 72, 74, 77, 79, 82, 84, 86 };   /* Bb major pentatonic */

static int swung(int s, int off)                 /* an offbeat sixteenth late */
{
    return s % 2 ? off_after(off, tp_sps() * SWING) : off;
}

static int lazy(int s, int off)
{
    return off_after(swung(s, off), tp_sps() * LAZY);
}

static void wow(tp_voice_t *v, int mask)          /* worn tape: the pitch drifting slowly sharp and flat */
{
    tv_lfo(v, LFO_DET, 0.45f + tp_rand() * 0.3f, 7.0f, mask);
}

/* ---- Keys ---- */
static void dusty_rhodes(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    int p = s % 16, i;
    if (p != 0 && p != 11)                    /* the chord on the one, and again pushed into beat four */
        return;
    if (p && tp_rand() > 0.6f)
        return;
    for (i = 0; i < 2; i++) {
        tp_voice_t *v = tv_new(l, swung(s, off));
        float f0 = tp_mtof(c->notes[i * 2 + 1]), f1 = tp_mtof(c->notes[i * 2 + 2]);
        tv_osc(v, W_SINE, f0, 0.0f, 1.0f);
        tv_osc(v, W_SINE, f1, 0.0f, 1.0f);
        tv_fm(v, 1.0f, f0 * 0.9f, 0.3f);
        tv_filter(v, F_LP, 1600.0f, 0.0f);
        tv_amp(v, 0.9f);
        tv_lfo(v, LFO_AMP, 3.5f, 0.12f, 0);
        wow(v, 3);
        tv_env_ad(v, 0.004f, p ? 0.04f : 0.055f, p ? 1.6f : 2.8f);
        tv_len(v, 3.0f);
    }
}

static void sub_bass(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    int p = s % 16, m;
    float f, len;
    tp_voice_t *v;
    if (p == 0) {                             /* the root, held; again on the and of three; a pickup now and then */
        m = c->root;
        len = tp_sps() * 7.0f;
    } else if (p == 10) {
        m = c->root;
        len = tp_sps() * 3.0f;
    } else if (p == 14 && tp_rand() < 0.45f) {
        m = c->root + 7;
        len = tp_sps() * 1.5f;
    } else
        return;
    while (m < 33)
        m += 12;
    while (m > 45)
        m -= 12;
    f = tp_mtof(m);
    v = tv_new(l, off);
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_osc(v, W_TRI, f * 2.0f, 0.0f, 0.12f);
    tv_filter(v, F_LP, 500.0f, 0.0f);
    tv_env_asr(v, 0.01f, 0.25f, len, 0.12f);
    tv_len(v, len + 0.2f);
}

static void tape_pad(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float bar = 16.0f * tp_sps();
    tp_voice_t *v;
    int i;
    if (s % 16)
        return;
    v = tv_new(l, off);                       /* a pad off an old cassette: dull, and wavering */
    for (i = 0; i < 4; i++)
        tv_osc(v, i % 2 ? W_TRI : W_SAW, tp_mtof(c->notes[i + 1]), i % 2 ? 6.0f : -6.0f, 1.0f);
    tv_filter(v, F_LP, 750.0f, 0.0f);
    tv_lfo(v, LFO_FC, 0.2f, 150.0f, 0);
    wow(v, 15);
    tv_env_asr(v, 1.2f, 0.045f, bar - 1.2f, 1.6f);
    tv_len(v, bar + 1.8f);
}

static void lazy_piano(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    int p = s % 16;
    float f0, f1;
    tp_voice_t *v;
    if ((p != 3 && p != 7 && p != 14) || tp_rand() > 0.45f)   /* a soft chord, here and there, off the beat */
        return;
    f0 = tp_mtof(c->notes[2] + 12);
    f1 = tp_mtof(c->notes[4] + 12);
    v = tv_new(l, swung(s, off));
    tv_osc(v, W_TRI, f0, 0.0f, 1.0f);
    tv_osc(v, W_TRI, f1, 0.0f, 0.9f);
    tv_osc(v, W_SINE, tp_mtof(c->notes[3] + 12), 0.0f, 0.8f);
    tv_filter(v, F_LP, 1300.0f, 0.0f);
    wow(v, 7);
    tv_env_ad(v, 0.003f, 0.07f, 1.3f);
    tv_len(v, 1.5f);
}

/* ---- Café ---- */
static void vinyl_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);           /* the record's surface, under everything */
    tv_persist(v);
    tv_noise(v, 0, 1.0f);
    tv_filter(v, F_BP, 4500.0f, 0.6f);
    tv_env_const(v, 0.05f);
}
static void vinyl(int l, int s, int off)
{
    int i;
    for (i = 0; i < 2; i++)                   /* crackles, and now and then a pop */
        if (tp_rand() < 0.55f)
            tp_burst(l, off_after(off, tp_rand() * tp_sps()), F_HP, 2500.0f + tp_rand() * 4000.0f, 0.7f, 0.0005f,
                     0.04f + tp_rand() * 0.08f, 0.004f, 0.01f);
    if (tp_rand() < 0.03f)
        tp_burst(l, off_after(off, tp_rand() * tp_sps()), F_BP, 900.0f, 1.0f, 0.0005f, 0.35f, 0.012f, 0.03f);
}

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
    tp_voice_t *v = tv_new(l, off);           /* the room talking, low: noise through two moving vowel bands */
    tv_persist(v);
    tv_noise(v, 1, 1.0f);
    tv_filter(v, F_BP, 480.0f, 4.0f);
    tv_filter(v, F_BP, 1300.0f, 5.0f);
    tv_filter_parallel(v);
    tv_env_const(v, 0.4f);
    tv_lfo(v, LFO_FC, 2.3f, 120.0f, 0);
    tv_lfo(v, LFO_GAIN, 0.7f, 0.18f, 0);
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

/* ---- Melodies ---- */
static void sleepy_piano(int l, int s, int off)
{
    int m = tp_kal[s % 64];
    float f;
    tp_voice_t *v;
    if (!m || tp_rand() > 0.65f)              /* the note grid, with gaps: half-remembered */
        return;
    f = tp_mtof(m - 12);
    v = tv_new(l, swung(s, off));
    tv_osc(v, W_TRI, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 2.0f, 0.0f, 0.35f);
    tv_osc(v, W_SINE, f * 3.0f, 0.0f, 0.1f);
    tv_filter(v, F_LP, 1800.0f, 0.0f);
    wow(v, 7);
    tv_env_ad(v, 0.002f, 0.07f, 1.4f);
    tv_part_decay(v, 2, 0.1f, 0.15f, 0.07f, 1.4f);
    tv_len(v, 1.6f);
}

static void mellow_guitar(int l, int s, int off)
{
    static const int8_t pat[16] = { 1, -1, 2, -1, 3, 4, -1, 2, -1, 3, -1, -1, 4, -1, 2, -1 };
    int idx = pat[s % 16];
    float f;
    tp_voice_t *v;
    if (idx < 0 || tp_rand() > 0.7f)          /* the chord picked out, string by string */
        return;
    f = tp_mtof(tp_chord_at(s)->notes[idx] + 12);
    v = tv_new(l, swung(s, off));             /* a clean electric, tone knob rolled off */
    tv_osc(v, W_TRI, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 2.0f, 4.0f, 0.3f);
    tv_filter(v, F_LP, 1500.0f, 0.0f);
    wow(v, 3);
    tv_env_ad(v, 0.003f, 0.065f, 0.9f);
    tv_len(v, 1.0f);
}

static void flute(int l, int s, int off)
{
    float len, f;
    tp_voice_t *v;
    if (s % 4 != 2 || tp_rand() > 0.22f || s % 64 >= 48)   /* a few long notes, then a breath */
        return;
    f = tp_mtof(P10[(int)(tp_rand() * 6.0f)]);
    len = tp_sps() * (tp_rand() < 0.5f ? 3.0f : 6.0f);
    v = tv_new(l, swung(s, off));
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_osc(v, W_TRI, f * 2.0f, 0.0f, 0.12f);
    tv_lfo(v, LFO_DET, 5.0f, 9.0f, 3);
    tv_lfo(v, LFO_AMP, 5.0f, 0.06f, 0);
    tv_noise(v, 0, 0.12f);                    /* air across the mouthpiece */
    tv_filter(v, F_LP, 2600.0f, 0.0f);
    tv_env_asr(v, 0.08f, 0.06f, len, 0.35f);
    tv_len(v, len + 0.5f);
}

static void music_box(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    int p = s % 32;
    float f;
    tp_voice_t *v;
    if (p > 6 || p % 2)                       /* a little rising figure every two bars, now and then a note missed */
        return;
    if (s % 64 >= 32 && tp_rand() > 0.6f)
        return;
    f = tp_mtof(c->notes[1 + p / 2] + 24);
    v = tv_new(l, off);
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 5.4f, 0.0f, 0.25f);
    tv_env_ad(v, 0.001f, 0.05f, 1.6f);
    tv_part_decay(v, 1, 0.03f, 0.12f, 0.05f, 1.6f);
    wow(v, 1);
    tv_len(v, 1.8f);
}

/* ---- Beats ---- */
static void boom_kick(int l, int s, int off)
{
    int p = s % 16;
    tp_voice_t *v;
    if (p != 0 && p != 10 && !(p == 7 && tp_rand() < 0.35f) && !(p == 3 && s % 64 >= 48 && tp_rand() < 0.5f))
        return;
    v = tv_new(l, swung(s, off));             /* round and padded, the beater's knock rolled off */
    tv_osc(v, W_SINE, 95.0f, 0.0f, 1.0f);
    tv_osc(v, W_TRI, 190.0f, 0.0f, 0.15f);
    tv_pitch_ramp(v, 48.0f, 0.09f);
    tv_filter(v, F_LP, 900.0f, 0.0f);
    tv_env_ad(v, 0.003f, p == 7 || p == 3 ? 0.45f : 0.75f, 0.38f);
    tv_len(v, 0.5f);
}

static void dusty_snare(int l, int s, int off)
{
    int p = s % 16, ghost;
    tp_voice_t *v;
    ghost = (p == 15 || p == 9) && tp_rand() < 0.25f;
    if (p != 4 && p != 12 && !ghost)
        return;
    v = tv_new(l, lazy(s, off));              /* a snare off an old record: the body, and the rattle, both dulled */
    tv_osc(v, W_SINE, 185.0f, 0.0f, 1.0f);
    tv_pitch_ramp(v, 160.0f, 0.05f);
    tv_env_ad(v, 0.001f, ghost ? 0.06f : 0.2f, 0.08f);
    tv_len(v, 0.12f);
    tp_burst(l, lazy(s, off), F_BP, 2000.0f, 0.8f, 0.001f, ghost ? 0.08f : 0.3f, ghost ? 0.05f : 0.13f, 0.2f);
}

static void lazy_hats(int l, int s, int off)
{
    int p = s % 16;
    float pk;
    if (s % 2 && tp_rand() > 0.35f)           /* eighths, the sixteenths between them now and then, swung */
        return;
    pk = s % 2 ? 0.045f : (p % 4 == 2 ? 0.11f : 0.075f);
    pk *= 0.8f + tp_rand() * 0.4f;
    if (p == 14 && tp_rand() < 0.3f)          /* the hat lifted, at the end of the bar */
        tp_burst(l, swung(s, off), F_HP, 6500.0f, 0.7f, 0.002f, 0.075f, 0.25f, 0.35f);
    else
        tp_burst(l, swung(s, off), F_BP, 7500.0f, 1.2f, 0.001f, pk, 0.03f, 0.06f);
}

static void snaps(int l, int s, int off)
{
    int p = s % 16;
    if (p != 4 && p != 12)
        return;
    if (s % 64 >= 48 && p == 12 && tp_rand() < 0.5f)   /* a breath at the end of the cycle */
        return;
    tp_burst(l, lazy(s, off), F_BP, 2300.0f, 3.0f, 0.001f, 0.55f, 0.035f, 0.08f);
    tp_burst(l, off_after(lazy(s, off), 0.004f), F_BP, 1200.0f, 2.0f, 0.001f, 0.22f, 0.02f, 0.05f);
}

static const tp_loop_t LOOPS[TP_NLOOP] = {
    /* name             fam  pan     rev    dly    fade  start              sched */
    { "Dusty Rhodes",   0, -0.15f, 0.4f,  0.1f,  1.0f,  0,                 dusty_rhodes },
    { "Sub bass",       0,  0.0f,  0.1f,  0.0f,  0.5f,  0,                 sub_bass },
    { "Tape pad",       0,  0.15f, 0.5f,  0.0f,  1.0f,  0,                 tape_pad },
    { "Lazy piano",     0,  0.25f, 0.45f, 0.2f,  0.5f,  0,                 lazy_piano },
    { "Vinyl crackle",  1,  0.0f,  0.1f,  0.0f,  1.0f,  vinyl_start,       vinyl },
    { "Rain on window", 1, -0.2f,  0.3f,  0.0f,  3.0f,  rain_window_start, rain_window },
    { "Murmur",         1,  0.15f, 0.35f, 0.0f,  3.0f,  murmur_start,      0 },
    { "Espresso",       1, -0.35f, 0.35f, 0.0f,  0.5f,  0,                 espresso },
    { "Sleepy piano",   2,  0.1f,  0.45f, 0.3f,  0.3f,  0,                 sleepy_piano },
    { "Mellow guitar",  2, -0.25f, 0.35f, 0.2f,  0.3f,  0,                 mellow_guitar },
    { "Flute",          2,  0.2f,  0.55f, 0.3f,  0.5f,  0,                 flute },
    { "Music box",      2, -0.1f,  0.5f,  0.35f, 0.3f,  0,                 music_box },
    { "Boom kick",      3,  0.0f,  0.1f,  0.0f,  0.05f, 0,                 boom_kick },
    { "Dusty snare",    3,  0.05f, 0.3f,  0.0f,  0.05f, 0,                 dusty_snare },
    { "Lazy hats",      3,  0.25f, 0.2f,  0.0f,  0.05f, 0,                 lazy_hats },
    { "Snaps",          3, -0.2f,  0.5f,  0.1f,  0.05f, 0,                 snaps },
};

const tp_bank_t TP_CAFE = { "Night Cafe", LOOPS, CHORDS, P10, 8, 101, 0.2f, { "Keys", "Cafe", "Melodies", "Beats" },
                            TP_STYLE_LUSH, 0 };
