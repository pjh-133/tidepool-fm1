/* Tidepool on the FM-1: Bank 11, Bonfire. Written for the FM-1 (no web app original), in the mood of a dark fantasy
 * of ruined kingdoms (Dark Souls): a fire to rest at among the ruins, in D minor. An ash drone, mournful strings, a
 * cathedral organ and a low choir; the bonfire, the ashen wind, a distant bell and chains; a harp, a solo cello,
 * soul glimmers and a lone soprano; timpani, an anvil, heavy steps and a sword drawn across stone. All its own:
 * nothing taken from the game's music. */
#include "tp_voice.h"
#include "tp_math.h"

#define SR TP_SR
#define off_after(off, sec) ((off) + (int)((sec) * SR))

static const tp_chord_t CHORDS[4] = {
    { "Dm", 38, { 50, 53, 57, 62, 65 } },
    { "Bbmaj7", 34, { 46, 50, 53, 57, 62 } },
    { "Gm", 43, { 43, 50, 55, 58, 62 } },
    { "Asus4", 45, { 45, 52, 57, 62, 64 } },
};
static const uint8_t P11[8] = { 62, 65, 67, 69, 72, 74, 77, 79 };   /* D minor pentatonic */

/* ---- Shrine ---- */
static void ash_drone(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);           /* D, low and dark, its filter breathing very slowly */
    tv_persist(v);
    tv_osc(v, W_SAW, tp_mtof(26), -7.0f, 0.5f);
    tv_osc(v, W_SAW, tp_mtof(26), 7.0f, 0.5f);
    tv_osc(v, W_SAW, tp_mtof(33), 0.0f, 0.2f);
    tv_osc(v, W_SINE, tp_mtof(14), 0.0f, 0.8f);
    tv_filter(v, F_LP, 240.0f, 1.5f);
    tv_env_const(v, 0.2f);
    tv_lfo(v, LFO_FC, 0.03f, 110.0f, 0);
}

static void mournful_strings(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    const int pick[3] = { c->notes[1] + 12, c->notes[2] + 12, c->notes[3] + 12 };
    float bar = 16.0f * tp_sps();
    tp_voice_t *v;
    int k;
    if (s % 16)
        return;
    v = tv_new(l, off);                       /* three notes in five saws, slow to rise, a slow vibrato */
    for (k = 0; k < 5; k++)
        tv_osc(v, W_SAW, tp_mtof(pick[k / 2]), k == 4 ? 0.0f : (float)(k % 2 * 2 - 1) * 8.0f, 1.0f);
    tv_lfo(v, LFO_DET, 4.8f, 7.0f, 0x1F);
    tv_filter(v, F_LP, 1700.0f, 0.7f);
    tv_env_asr(v, 2.2f, 0.043f, bar - 2.2f, 2.4f);
    tv_len(v, bar + 2.6f);
}

static void cathedral_organ(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float len = 32.0f * tp_sps();
    tp_voice_t *v;
    if (s % 32)
        return;
    v = tv_new(l, off);                       /* a full chord over a sixteen-foot root, slow to speak in the nave */
    tv_osc(v, W_SINE, tp_mtof(c->root), 0.0f, 1.0f);
    tv_osc(v, W_SQR, tp_mtof(c->notes[0] + 12), 0.0f, 0.6f);
    tv_osc(v, W_SQR, tp_mtof(c->notes[2] + 12), 0.0f, 0.6f);
    tv_osc(v, W_TRI, tp_mtof(c->notes[3] + 12), 0.0f, 0.9f);
    tv_osc(v, W_TRI, tp_mtof(c->notes[4] + 12), 0.0f, 0.7f);
    tv_filter(v, F_LP, 1300.0f, 0.7f);
    tv_env_asr(v, 0.9f, 0.032f, len - 0.9f, 2.5f);
    tv_len(v, len + 2.7f);
}

static void low_choir(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float len = 32.0f * tp_sps();
    tp_voice_t *v;
    int i;
    if (s % 32 != 16)
        return;
    v = tv_new(l, off);                       /* men's voices on "oh": two formants side by side */
    for (i = 0; i < 3; i++) {
        tv_osc(v, W_SAW, tp_mtof(c->notes[i]), 0.0f, 1.0f);
        tv_lfo(v, LFO_DET, 4.6f + tp_rand() * 0.6f, 9.0f, 1 << i);
    }
    tv_filter(v, F_BP, 480.0f, 4.0f);
    tv_filter(v, F_BP, 820.0f, 4.0f);
    tv_filter_parallel(v);
    tv_env_asr(v, 3.0f, 0.165f, len - 3.0f, 3.5f);
    tv_len(v, len + 3.7f);
}

/* ---- Ruins ---- */
static void bonfire_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);           /* the fire's low roar, rising and falling */
    tv_persist(v);
    tv_noise(v, 1, 1.0f);
    tv_filter(v, F_LP, 600.0f, 0.7f);
    tv_env_const(v, 0.16f);
    tv_lfo(v, LFO_GAIN, 0.11f, 0.06f, 0);
}
static void bonfire(int l, int s, int off)
{
    int n = tp_rand() < 0.6f ? 1 + (int)(tp_rand() * 3.0f) : 0, i;
    for (i = 0; i < n; i++) {                 /* crackles, and now and then a log settling */
        int at = off_after(off, tp_rand() * tp_sps());
        float f = 1500.0f + tp_rand() * 4500.0f, pk = 0.04f + tp_rand() * 0.08f, d = 0.008f + tp_rand() * 0.025f;
        tp_burst(l, at, F_BP, f, 2.0f, 0.001f, pk, d, 0.05f);
    }
    if (s % 16 == 9 && tp_rand() < 0.2f)
        tp_burst(l, off, F_LP, 300.0f, 0.7f, 0.01f, 0.25f, 0.35f, 0.5f);
}

static void ashen_wind_start(int l, int off)
{
    tp_voice_t *v = tv_new(l, off);           /* a low wind through broken arches, gusting */
    tv_persist(v);
    tv_noise(v, 1, 1.0f);
    tv_filter(v, F_BP, 320.0f, 2.2f);
    tv_env_const(v, 0.55f);
    tv_lfo(v, LFO_FC, 0.05f, 180.0f, 0);
    tv_lfo(v, LFO_GAIN, 0.07f, 0.3f, 0);
    v = tv_new(l, off);                       /* and its whistle, high and thin */
    tv_persist(v);
    tv_noise(v, 0, 1.0f);
    tv_filter(v, F_BP, 1100.0f, 18.0f);
    tv_env_const(v, 0.05f);
    tv_lfo(v, LFO_FC, 0.06f, 260.0f, 0);
}

static void distant_bell(int l, int s, int off)
{
    float f;
    tp_voice_t *v;
    if (s % 32 || tp_rand() > 0.55f)
        return;
    f = tp_mtof(tp_chord_at(s)->root + 12);
    v = tv_new(l, off);                       /* a great bell, far off: its hum and its strike tone */
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 0.5f, 0.0f, 0.6f);
    tv_osc(v, W_SINE, f * 2.4f, 0.0f, 0.25f);
    tv_fm(v, 1.41f, f * 0.9f, 3.0f);
    tv_filter(v, F_LP, 1800.0f, 0.7f);
    tv_env_ad(v, 0.004f, 0.12f, 8.0f);
    tv_len(v, 8.2f);
}

static void chains(int l, int s, int off)
{
    int n, i, at;
    tp_voice_t *v;
    if (tp_rand() > 0.035f)
        return;
    at = off_after(off, tp_rand() * tp_sps());
    n = 4 + (int)(tp_rand() * 5.0f);
    for (i = 0; i < n; i++) {                 /* links knocking against each other, settling */
        float f = 2600.0f + tp_rand() * 3600.0f;
        tp_burst(l, off_after(at, (float)i * (0.03f + tp_rand() * 0.05f)), F_BP, f, 9.0f, 0.001f,
                 0.2f - (float)i * 0.016f, 0.06f, 0.1f);
    }
    v = tv_new(l, at);                        /* the iron's ring under them */
    tv_osc(v, W_SINE, 1870.0f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, 2950.0f, 0.0f, 0.6f);
    tv_env_ad(v, 0.002f, 0.045f, 0.6f);
    tv_len(v, 0.7f);
}

/* ---- Lament ---- */
static void harp(int l, int s, int off)
{
    int m = tp_kal[s % 64];
    float f;
    tp_voice_t *v;
    if (!m || s % 2)
        return;
    f = tp_mtof(m);
    v = tv_new(l, off);                       /* a plucked gut string, soft and ringing */
    tv_osc(v, W_TRI, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 2.0f, 0.0f, 0.3f);
    tv_filter(v, F_LP, 3000.0f, 0.7f);
    tv_filter_ramp(v, 1, 1000.0f, 0.6f);
    tv_env_ad(v, 0.002f, 0.08f, 2.2f);
    tv_len(v, 2.3f);
}

static void solo_cello(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float len = tp_sps() * 7.0f, f;
    tp_voice_t *v;
    if (s % 8 || tp_rand() > 0.65f)
        return;
    f = tp_mtof(c->notes[1 + (int)(tp_rand() * 4.0f)] - 12 + 12 * (s % 16 == 8));
    v = tv_new(l, off);                       /* a long bowed note, its vibrato coming in */
    tv_osc(v, W_SAW, f, 0.0f, 1.0f);
    tv_osc(v, W_SAW, f, 6.0f, 0.5f);
    tv_lfo(v, LFO_DET, 5.2f, 14.0f, 3);
    tv_filter(v, F_LP, 1300.0f, 1.0f);
    tv_env_asr(v, 0.5f, 0.06f, len, 1.4f);
    tv_len(v, len + 1.6f);
}

static void soul_glimmer(int l, int s, int off)
{
    int n, i, k;
    if (s % 32 != 8 || tp_rand() > 0.6f)
        return;
    n = 3 + (int)(tp_rand() * 3.0f);
    k = (int)(tp_rand() * 3.0f);
    for (i = 0; i < n; i++) {                 /* a few bright notes, rising, as something is taken in */
        float f = tp_mtof(P11[(k + i) % 8] + 12 + 12 * ((k + i) / 8));
        tp_voice_t *v = tv_new(l, off_after(off, (float)i * tp_sps() * 0.5f));
        tv_osc(v, W_SINE, f, 0.0f, 1.0f);
        tv_osc(v, W_SINE, f * 3.0f, 0.0f, 0.15f);
        tv_env_ad(v, 0.005f, 0.11f, 1.8f);
        tv_len(v, 1.9f);
    }
}

static void lone_soprano(int l, int s, int off)
{
    const tp_chord_t *c = tp_chord_at(s);
    float len = tp_sps() * 12.0f, f;
    tp_voice_t *v;
    if (s % 64 != 32 || tp_rand() > 0.7f)
        return;
    f = tp_mtof(c->notes[2 + (int)(tp_rand() * 3.0f)] + 12);
    v = tv_new(l, off);                       /* a woman's voice, wordless, on "ah" */
    tv_osc(v, W_SAW, f, 0.0f, 1.0f);
    tv_osc(v, W_TRI, f, 0.0f, 0.8f);
    tv_lfo(v, LFO_DET, 5.4f, 18.0f, 3);
    tv_filter(v, F_BP, 820.0f, 5.0f);
    tv_filter(v, F_BP, 1250.0f, 6.0f);
    tv_filter_parallel(v);
    tv_env_asr(v, 1.2f, 0.17f, len - 1.2f, 2.5f);
    tv_len(v, len + 2.7f);
}

/* ---- Battle ---- */
static void timpani(int l, int s, int off)
{
    int p = s % 16;
    float f;
    tp_voice_t *v;
    if (p != 0 && !(p == 14 && s % 32 == 30))
        return;
    f = tp_mtof(tp_chord_at(s)->root + (tp_chord_at(s)->root < 40 ? 12 : 0));
    v = tv_new(l, off);                       /* tuned to the chord, a soft mallet */
    tv_osc(v, W_SINE, f * 1.03f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 1.5f, 0.0f, 0.3f);
    tv_pitch_ramp(v, f, 0.15f);
    tv_env_ad(v, 0.004f, p ? 0.3f : 0.5f, 1.6f);
    tv_len(v, 1.7f);
    tp_burst(l, off, F_LP, 500.0f, 0.7f, 0.002f, 0.12f, 0.08f, 0.15f);
}

static void anvil(int l, int s, int off)
{
    tp_voice_t *v;
    if (s % 16 != 8 || tp_rand() > 0.6f)
        return;
    v = tv_new(l, off);                       /* a hammer on the anvil, far down the hall */
    tv_osc(v, W_SINE, 1180.0f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, 3020.0f, 0.0f, 0.4f);
    tv_fm(v, 3.7f, 700.0f, 0.2f);
    tv_env_ad(v, 0.001f, 0.09f, 1.3f);
    tv_len(v, 1.4f);
    tp_burst(l, off, F_BP, 4200.0f, 2.0f, 0.001f, 0.1f, 0.03f, 0.06f);
}

static void heavy_steps(int l, int s, int off)
{
    tp_voice_t *v;
    if (s % 4)
        return;
    v = tv_new(l, off);                       /* a slow tread in armour: a thud, and the plates shifting */
    tv_osc(v, W_SINE, 72.0f, 0.0f, 1.0f);
    tv_pitch_ramp(v, 45.0f, 0.12f);
    tv_env_ad(v, 0.003f, s % 8 ? 0.16f : 0.22f, 0.25f);
    tv_len(v, 0.3f);
    tp_burst(l, off, F_LP, 380.0f, 0.7f, 0.002f, 0.13f, 0.08f, 0.12f);
    tp_burst(l, off_after(off, 0.03f), F_BP, 5200.0f, 5.0f, 0.001f, 0.035f, 0.05f, 0.08f);
}

static void sword_scrape(int l, int s, int off)
{
    tp_voice_t *v;
    if (s % 32 != 20 || tp_rand() > 0.5f)
        return;
    v = tv_new(l, off);                       /* a blade drawn along stone, then ringing */
    tv_noise(v, 0, 1.0f);
    tv_filter(v, F_BP, 2000.0f, 6.0f);
    tv_filter_ramp(v, 1, 6500.0f, 0.6f);
    tv_env_asr(v, 0.1f, 0.09f, 0.35f, 0.15f);
    tv_len(v, 0.65f);
    v = tv_new(l, off_after(off, 0.55f));
    tv_osc(v, W_SINE, 2640.0f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, 2652.0f, 0.0f, 0.8f);
    tv_env_ad(v, 0.002f, 0.05f, 2.2f);
    tv_len(v, 2.3f);
}

static const tp_loop_t LOOPS[TP_NLOOP] = {
    /* name              fam  pan     rev    dly    fade  start             sched */
    { "Ash drone",        0,  0.0f,  0.35f, 0.0f,  4.0f,  ash_drone,        0 },
    { "Mournful strings", 0, -0.15f, 0.6f,  0.0f,  1.0f,  0,                mournful_strings },
    { "Cathedral organ",  0,  0.1f,  0.7f,  0.0f,  1.0f,  0,                cathedral_organ },
    { "Low choir",        0, -0.05f, 0.7f,  0.1f,  1.0f,  0,                low_choir },
    { "Bonfire",          1,  0.1f,  0.25f, 0.0f,  2.0f,  bonfire_start,    bonfire },
    { "Ashen wind",       1, -0.2f,  0.35f, 0.0f,  3.0f,  ashen_wind_start, 0 },
    { "Distant bell",     1,  0.35f, 0.75f, 0.15f, 0.5f,  0,                distant_bell },
    { "Chains",           1, -0.35f, 0.5f,  0.2f,  0.3f,  0,                chains },
    { "Harp",             2, -0.2f,  0.5f,  0.3f,  0.3f,  0,                harp },
    { "Solo cello",       2,  0.15f, 0.55f, 0.15f, 0.5f,  0,                solo_cello },
    { "Soul glimmer",     2,  0.3f,  0.65f, 0.4f,  0.3f,  0,                soul_glimmer },
    { "Lone soprano",     2,  0.0f,  0.75f, 0.3f,  0.5f,  0,                lone_soprano },
    { "Timpani",          3,  0.0f,  0.5f,  0.0f,  0.05f, 0,                timpani },
    { "Anvil",            3,  0.3f,  0.6f,  0.25f, 0.05f, 0,                anvil },
    { "Heavy steps",      3, -0.15f, 0.4f,  0.0f,  0.05f, 0,                heavy_steps },
    { "Sword scrape",     3,  0.25f, 0.55f, 0.2f,  0.05f, 0,                sword_scrape },
};

const tp_bank_t TP_BONFIRE = { "Bonfire", LOOPS, CHORDS, P11, 8, 91, 0.3f, { "Shrine", "Ruins", "Lament", "Battle" },
                               TP_STYLE_TRIAD, 0 };
