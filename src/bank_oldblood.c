/* Tidepool on the FM-1: Bank 12, Old Blood. Written for the FM-1 (no web app original), in the mood of a gothic
 * Victorian nightmare (Bloodborne): a cursed city on the night of the hunt, in F minor, its fourth chord C major
 * for the harmonic minor's dread. A dread drone, a gothic choir, a pipe organ and tremolo strings; night fog,
 * church bells, crows and rain on the cobbles; a warped lullaby box, a violin's lament, a harpsichord and an eerie
 * voice; low toms in a lurching three, a knock at a coffin lid, cello stabs and a rattle of bones. All its own:
 * nothing taken from the game's music. */
#include "tp_voice.h"
#include "tp_math.h"

#define SR TP_SR
#define off_after(off, sec) ((off) + (int)((sec) * SR))

static const tp_chord_t CHORDS[4] = {
    { "Fm", 41, { 53, 56, 60, 65, 68 } },
    { "Db", 37, { 49, 53, 56, 61, 65 } },
    { "Bbm", 46, { 46, 53, 58, 61, 65 } },
    { "C", 36, { 48, 52, 55, 60, 64 } },
};
static const uint8_t P12[8] = { 65, 67, 68, 72, 73, 77, 79, 80 };   /* F minor, without its E flat */

/* ---- Cathedral ---- */
static void dread_drone(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);           /* F, with a faint tritone above it, slowly pulling out of tune */
    tv_persist(v);
    tv_osc(v, W_SAW, tp_mtof(29), -5.0f, 0.5f);
    tv_osc(v, W_SAW, tp_mtof(29), 5.0f, 0.5f);
    tv_osc(v, W_SINE, tp_mtof(17), 0.0f, 0.8f);
    tv_osc(v, W_TRI, tp_mtof(35), 0.0f, 0.12f);
    tv_lfo(v, LFO_DET, 0.05f, 9.0f, 3);
    tv_filter(v, F_LP, 280.0f, 2.0f);
    tv_env_const(v, 0.19f);
    tv_lfo(v, LFO_FC, 0.04f, 120.0f, 0);
}

static void gothic_choir(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float len = 32.0f * tp_sps();
    tp_voice_t *v;
    int i;
    if (s % 32)
        return;
    v = tv_new(l, off);                       /* a full choir on "ah", swelling and falling back */
    for (i = 0; i < 4; i++) {
        tv_osc(v, W_SAW, tp_mtof(c->notes[1 + i]), (float)(i - 2) * 4.0f, 1.0f);
        tv_lfo(v, LFO_DET, 4.5f + tp_rand() * 0.8f, 10.0f, 1 << i);
    }
    tv_filter(v, F_BP, 700.0f, 4.0f);
    tv_filter(v, F_BP, 1150.0f, 4.0f);
    tv_filter_parallel(v);
    tv_env_asr(v, len * 0.3f, 0.14f, len * 0.35f, len * 0.4f);
    tv_len(v, len * 1.1f);
}

static void pipe_organ(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float bar = 16.0f * tp_sps();
    tp_voice_t *v;
    if (s % 16)
        return;
    v = tv_new(l, off);                       /* reeds over a pedal note: hard and bright */
    tv_osc(v, W_SINE, tp_mtof(c->root), 0.0f, 1.0f);
    tv_osc(v, W_SQR, tp_mtof(c->notes[0] + 12), 0.0f, 0.7f);
    tv_osc(v, W_SQR, tp_mtof(c->notes[1] + 12), 0.0f, 0.7f);
    tv_osc(v, W_SAW, tp_mtof(c->notes[2] + 12), 0.0f, 0.4f);
    tv_osc(v, W_SQR, tp_mtof(c->notes[0] + 24), 0.0f, 0.35f);
    tv_filter(v, F_LP, 1700.0f, 0.8f);
    tv_env_asr(v, 0.35f, 0.033f, bar - 0.35f, 1.6f);
    tv_len(v, bar + 1.8f);
}

static void tremolo_strings(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    const int pick[3] = { c->notes[1] + 12, c->notes[2] + 12, c->notes[4] + 12 };
    float bar = 16.0f * tp_sps();
    tp_voice_t *v;
    int k;
    if (s % 16)
        return;
    v = tv_new(l, off);                       /* bows shaking on the string: the dread before something comes */
    for (k = 0; k < 5; k++)
        tv_osc(v, W_SAW, tp_mtof(pick[k / 2]), k == 4 ? 0.0f : (float)(k % 2 * 2 - 1) * 10.0f, 1.0f);
    tv_amp(v, 0.65f);
    tv_lfo(v, LFO_AMP, 11.0f, 0.35f, 0);
    tv_filter(v, F_LP, 2600.0f, 0.7f);
    tv_env_asr(v, 1.8f, 0.075f, bar - 1.8f, 1.8f);
    tv_len(v, bar + 2.0f);
}

/* ---- Streets ---- */
static void night_fog_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);           /* still air in narrow streets: a low murmur and a soft hiss */
    tv_persist(v);
    tv_noise(v, 1, 1.0f);
    tv_filter(v, F_LP, 220.0f, 0.7f);
    tv_env_const(v, 0.18f);
    tv_lfo(v, LFO_GAIN, 0.05f, 0.15f, 0);
    v = tv_new(l, off);
    tv_persist(v);
    tv_noise(v, 0, 1.0f);
    tv_filter(v, F_BP, 700.0f, 1.2f);
    tv_env_const(v, 0.04f);
    tv_lfo(v, LFO_FC, 0.03f, 300.0f, 0);
}

static void church_bells(int l, int s, int off)
{
    int i;
    if (s % 32 || tp_rand() > 0.5f)
        return;
    for (i = 0; i < 2; i++) {                 /* two bells from the cathedral, a fifth apart, the second late */
        float f = tp_mtof(tp_chord_at(s)->root + 24 + 7 * i);
        tp_voice_t *v = tv_new(l, off_after(off, (float)i * tp_sps() * 3.0f));
        tv_osc(v, W_SINE, f, 0.0f, 1.0f);
        tv_osc(v, W_SINE, f * 0.5f, 0.0f, 0.5f);
        tv_osc(v, W_SINE, f * 2.53f, 0.0f, 0.2f);
        tv_fm(v, 1.41f, f * 1.1f, 2.5f);
        tv_filter(v, F_LP, 2400.0f, 0.7f);
        tv_env_ad(v, 0.003f, 0.11f, 6.0f);
        tv_len(v, 6.2f);
    }
}

static void crows(int l, int s, int off)
{
    int n, i, at;
    if (tp_rand() > 0.03f)
        return;
    at = off_after(off, tp_rand() * tp_sps());
    n = 1 + (int)(tp_rand() * 3.0f);
    for (i = 0; i < n; i++) {                 /* caws: a hoarse cry falling, two or three in a row */
        float f = 620.0f + tp_rand() * 160.0f;
        tp_voice_t *v = tv_new(l, off_after(at, (float)i * 0.32f));
        tv_osc(v, W_SAW, f, 0.0f, 1.0f);
        tv_osc(v, W_SQR, f * 1.5f, 13.0f, 0.5f);
        tv_pitch_ramp(v, f * 0.7f, 0.22f);
        tv_filter(v, F_BP, 1300.0f, 2.5f);
        tv_env_ad(v, 0.015f, 0.18f, 0.2f);
        tv_len(v, 0.26f);
        tp_burst(l, off_after(at, (float)i * 0.32f), F_BP, 2200.0f, 1.5f, 0.01f, 0.07f, 0.15f, 0.2f);
    }
}

static void rain_cobbles_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);
    tv_persist(v);
    tv_noise(v, 0, 1.0f);
    tv_filter(v, F_BP, 2800.0f, 0.8f);
    tv_env_const(v, 0.07f);
    tv_lfo(v, LFO_GAIN, 0.06f, 0.025f, 0);
}
static void rain_cobbles(int l, int s, int off)
{
    int n = (int)(tp_rand() * 3.0f), i;
    for (i = 0; i < n; i++) {                 /* drops on stone and in the puddles */
        int at = off_after(off, tp_rand() * tp_sps());
        tp_burst(l, at, F_BP, 1400.0f + tp_rand() * 3000.0f, 5.0f, 0.001f, 0.04f + tp_rand() * 0.05f, 0.025f, 0.05f);
    }
}

/* ---- Nightmare ---- */
static void lullaby_box(int l, int s, int off)
{
    int m = tp_kal[s % 64];
    float f;
    tp_voice_t *v;
    if (!m)
        return;
    f = tp_mtof(m + 12);
    v = tv_new(l, off);                       /* a music box's comb, its spring running down: the pitch wavers */
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 4.02f, 0.0f, 0.25f);
    tv_lfo(v, LFO_DET, 0.7f, 16.0f, 3);
    tv_env_ad(v, 0.001f, 0.075f, 1.3f);
    tv_len(v, 1.4f);
}

static void violin_lament(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float len = tp_sps() * 5.0f, f;
    tp_voice_t *v;
    if (s % 16 || tp_rand() > 0.75f)
        return;
    f = tp_mtof(c->notes[1 + (int)(tp_rand() * 4.0f)] + 12);
    v = tv_new(l, off);                       /* sliding up into the note, a wide vibrato */
    tv_osc(v, W_SAW, f * 0.95f, 0.0f, 1.0f);
    tv_osc(v, W_SAW, f * 0.95f, 5.0f, 0.4f);
    tv_pitch_ramp(v, f, 0.25f);
    tv_lfo(v, LFO_DET, 6.0f, 20.0f, 3);
    tv_filter(v, F_LP, 2800.0f, 1.0f);
    tv_env_asr(v, 0.3f, 0.05f, len, 1.2f);
    tv_len(v, len + 1.4f);
}

static void harpsichord(int l, int s, int off)
{
    static const uint8_t UPDOWN[8] = { 0, 1, 2, 3, 4, 3, 2, 1 };
    const tp_chord_t *c = tp_chord_at(s);
    float f;
    tp_voice_t *v;
    if (s % 2 || (s % 64 >= 48 && s % 4))
        return;
    f = tp_mtof(c->notes[UPDOWN[(s / 2) % 8]] + 12);
    v = tv_new(l, off);                       /* a quill plucking: bright, quick to fade */
    tv_osc(v, W_SAW, f, 0.0f, 1.0f);
    tv_osc(v, W_SQR, f * 2.0f, 0.0f, 0.3f);
    tv_filter(v, F_LP, 5200.0f, 1.0f);
    tv_filter_ramp(v, 1, 1500.0f, 0.25f);
    tv_env_ad(v, 0.001f, 0.1f, 0.6f);
    tv_len(v, 0.7f);
}

static void eerie_voice(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float len = tp_sps() * 10.0f, f;
    tp_voice_t *v;
    if (s % 64 != 40 || tp_rand() > 0.6f)
        return;
    f = tp_mtof(c->notes[3] + 12);
    v = tv_new(l, off);                       /* a voice in the dark, sinking a semitone as it holds */
    tv_osc(v, W_SAW, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_pitch_ramp(v, f * 0.944f, len);
    tv_lfo(v, LFO_DET, 3.2f, 14.0f, 3);
    tv_filter(v, F_BP, 900.0f, 5.0f);
    tv_filter(v, F_BP, 1350.0f, 6.0f);
    tv_filter_parallel(v);
    tv_env_asr(v, 1.6f, 0.15f, len - 1.6f, 2.5f);
    tv_len(v, len + 2.7f);
}

/* ---- Hunt ---- */
static void low_toms(int l, int s, int off)
{
    int p = s % 16;
    tp_voice_t *v;
    if (p != 0 && p != 6 && p != 12)
        return;
    v = tv_new(l, off);                       /* three, three, two: a stagger, not a march */
    tv_osc(v, W_SINE, p ? 120.0f : 100.0f, 0.0f, 1.0f);
    tv_pitch_ramp(v, p ? 78.0f : 62.0f, 0.2f);
    tv_env_ad(v, 0.003f, p ? 0.3f : 0.45f, 0.6f);
    tv_len(v, 0.7f);
    tp_burst(l, off, F_LP, 900.0f, 0.7f, 0.002f, 0.1f, 0.05f, 0.1f);
}

static void coffin_knock(int l, int s, int off)
{
    int i;
    if (s % 32 != 26 || tp_rand() > 0.7f)
        return;
    for (i = 0; i < 2; i++) {                 /* two knocks on a hollow lid */
        tp_voice_t *v = tv_new(l, off_after(off, (float)i * 0.14f));
        tv_osc(v, W_SINE, 240.0f, 0.0f, 1.0f);
        tv_osc(v, W_SINE, 610.0f, 0.0f, 0.4f);
        tv_pitch_ramp(v, 200.0f, 0.06f);
        tv_env_ad(v, 0.001f, 0.32f - (float)i * 0.08f, 0.18f);
        tv_len(v, 0.22f);
        tp_burst(l, off_after(off, (float)i * 0.14f), F_BP, 1100.0f, 2.0f, 0.001f, 0.1f, 0.03f, 0.05f);
    }
}

static void cello_stabs(int l, int s, int off)
{
    static const uint8_t UP[4] = { 0, 0, 7, 12 };
    const tp_chord_t *c = tp_chord_at(s);
    float f;
    tp_voice_t *v;
    if (s % 4)
        return;
    f = tp_mtof(c->root + 12 + UP[(s / 4) % 4]);
    v = tv_new(l, off);                       /* short, hard bows on the root: the hunt's pulse */
    tv_osc(v, W_SAW, f, 0.0f, 1.0f);
    tv_osc(v, W_SAW, f, 9.0f, 0.6f);
    tv_filter(v, F_LP, 1100.0f, 1.5f);
    tv_filter_ramp(v, 1, 400.0f, 0.2f);
    tv_env_ad(v, 0.006f, s % 16 ? 0.11f : 0.15f, 0.25f);
    tv_len(v, 0.3f);
}

static void bone_rattle(int l, int s, int off)
{
    int p = s % 16, n, i;
    if ((p != 4 && p != 10) || tp_rand() > 0.8f)
        return;
    n = 3 + (int)(tp_rand() * 3.0f);
    for (i = 0; i < n; i++)                   /* dry clicks tumbling together */
        tp_burst(l, off_after(off, (float)i * (0.02f + tp_rand() * 0.02f)), F_BP, 2200.0f + tp_rand() * 2600.0f,
                 4.0f, 0.001f, 0.45f - (float)i * 0.05f, 0.018f, 0.04f);
}

static const tp_loop_t LOOPS[TP_NLOOP] = {
    /* name              fam  pan     rev    dly    fade  start               sched */
    { "Dread drone",      0,  0.0f,  0.4f,  0.0f,  4.0f,  dread_drone,        0 },
    { "Gothic choir",     0,  0.05f, 0.75f, 0.1f,  1.0f,  0,                  gothic_choir },
    { "Pipe organ",       0, -0.1f,  0.7f,  0.0f,  1.0f,  0,                  pipe_organ },
    { "Tremolo strings",  0,  0.15f, 0.55f, 0.0f,  1.0f,  0,                  tremolo_strings },
    { "Night fog",        1,  0.0f,  0.3f,  0.0f,  3.0f,  night_fog_start,    0 },
    { "Church bells",     1,  0.3f,  0.75f, 0.15f, 0.5f,  0,                  church_bells },
    { "Crows",            1, -0.4f,  0.5f,  0.2f,  0.3f,  0,                  crows },
    { "Rain on cobbles",  1,  0.1f,  0.3f,  0.0f,  3.0f,  rain_cobbles_start, rain_cobbles },
    { "Lullaby box",      2, -0.2f,  0.55f, 0.35f, 0.3f,  0,                  lullaby_box },
    { "Violin lament",    2,  0.2f,  0.6f,  0.25f, 0.5f,  0,                  violin_lament },
    { "Harpsichord",      2, -0.1f,  0.45f, 0.15f, 0.3f,  0,                  harpsichord },
    { "Eerie voice",      2,  0.1f,  0.8f,  0.3f,  0.5f,  0,                  eerie_voice },
    { "Low toms",         3,  0.0f,  0.5f,  0.0f,  0.05f, 0,                  low_toms },
    { "Coffin knock",     3, -0.3f,  0.55f, 0.25f, 0.05f, 0,                  coffin_knock },
    { "Cello stabs",      3,  0.1f,  0.4f,  0.0f,  0.05f, 0,                  cello_stabs },
    { "Bone rattle",      3,  0.3f,  0.45f, 0.2f,  0.05f, 0,                  bone_rattle },
};

const tp_bank_t TP_OLDBLOOD = { "Old Blood", LOOPS, CHORDS, P12, 8, 37, 0.32f,
                                { "Cathedral", "Streets", "Nightmare", "Hunt" }, TP_STYLE_TRIAD, 0 };
