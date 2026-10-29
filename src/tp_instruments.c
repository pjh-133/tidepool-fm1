/* Tidepool on the FM-1: the Blank Canvas's instruments. The web app's 19 melodic ones (VOICES, ids 0..18, in
 * public/index.html) and its note (voiceNote): the same oscillators, filters, FM and envelopes; velocity sets the
 * level and opens the low-pass. Then 17 written for the FM-1 (ids 19..35), each its own function: two drum kits (a
 * sound a key, the octave ignored) and instruments from the preset banks, with the slides, vibrato and noise a table
 * entry has no room for. */
#include "tp_canvas.h"
#include "tp_math.h"

typedef struct {
    const char *name;
    uint8_t nw;
    struct { uint8_t wave; float ratio, det, lvl; } w[3];
    float fm_r, fm_i, fm_dec;                    /* fm_r 0: none */
    float lp, bp_f, bp_q;                        /* a low-pass (lp), or a band-pass (bp_f, bp_q), or neither */
    uint8_t sus;                                 /* held (attack, release) or struck (attack, decay) */
    float a, d, r, gain;
} instr_t;

static const instr_t INSTR[19] = {
    { "Glass bell", 1, { { W_SINE, 1, 0, 1 } }, 3.5f, 2.0f, 0.5f, 0, 0, 0, 0, 0.002f, 2.4f, 0, 0.12f },
    { "Kalimba", 2, { { W_TRI, 1, 0, 1 }, { W_SINE, 3.01f, 0, 0.28f } }, 0, 0, 0, 3200, 0, 0, 0, 0.002f, 1.0f, 0, 0.18f },
    { "Felt keys", 1, { { W_SINE, 1, 0, 1 } }, 1.0f, 1.1f, 0.5f, 0, 0, 0, 0, 0.003f, 1.9f, 0, 0.14f },
    { "Pluck", 2, { { W_SAW, 1, -5, 0.5f }, { W_SAW, 1, 5, 0.5f } }, 0, 0, 0, 2200, 0, 0, 0, 0.002f, 0.55f, 0, 0.09f },
    { "Soft pad", 3, { { W_SAW, 1, -7, 0.5f }, { W_SAW, 1, 7, 0.5f }, { W_TRI, 0.5f, 0, 0.5f } }, 0, 0, 0, 1500, 0, 0, 1,
      0.6f, 0, 1.6f, 0.06f },
    { "Warm organ", 2, { { W_SQR, 1, 0, 0.5f }, { W_SINE, 2, 0, 0.5f } }, 0, 0, 0, 1300, 0, 0, 1, 0.03f, 0, 0.25f, 0.05f },
    { "Choir", 2, { { W_SAW, 1, -9, 0.5f }, { W_SAW, 1, 9, 0.5f } }, 0, 0, 0, 0, 900, 2, 1, 0.5f, 0, 1.2f, 0.1f },
    { "Sub bass", 2, { { W_SINE, 1, 0, 1 }, { W_TRI, 1, 0, 0.4f } }, 0, 0, 0, 600, 0, 0, 1, 0.01f, 0, 0.2f, 0.35f },
    { "Cello", 2, { { W_SAW, 1, -6, 0.5f }, { W_SAW, 1, 6, 0.5f } }, 0, 0, 0, 900, 0, 0, 1, 0.25f, 0, 0.5f, 0.09f },
    { "Chip lead", 1, { { W_SQR, 1, 0, 1 } }, 0, 0, 0, 3000, 0, 0, 1, 0.005f, 0, 0.12f, 0.07f },
    { "Flute", 2, { { W_SINE, 1, 0, 1 }, { W_SINE, 2, 0, 0.12f } }, 0, 0, 0, 0, 0, 0, 1, 0.09f, 0, 0.3f, 0.2f },
    { "Harp", 2, { { W_TRI, 1, 0, 1 }, { W_SINE, 2, 0, 0.3f } }, 0, 0, 0, 3600, 0, 0, 0, 0.002f, 1.5f, 0, 0.2f },
    { "Mellow brass", 2, { { W_SAW, 1, -4, 0.5f }, { W_SAW, 1, 4, 0.5f } }, 0, 0, 0, 1100, 0, 0, 1, 0.08f, 0, 0.3f, 0.08f },
    { "Marimba", 2, { { W_SINE, 1, 0, 1 }, { W_SINE, 4, 0, 0.35f } }, 0, 0, 0, 0, 0, 0, 0, 0.002f, 0.55f, 0, 0.3f },
    { "Moog bass", 2, { { W_SAW, 1, 0, 0.6f }, { W_SQR, 0.5f, 0, 0.5f } }, 0, 0, 0, 520, 0, 0, 1, 0.005f, 0, 0.12f, 0.28f },
    { "Music box", 2, { { W_SINE, 1, 0, 1 }, { W_SINE, 2.76f, 0, 0.15f } }, 0, 0, 0, 0, 0, 0, 0, 0.001f, 1.8f, 0, 0.14f },
    { "Nylon guitar", 2, { { W_TRI, 1, 0, 1 }, { W_SINE, 2, 0, 0.3f } }, 0, 0, 0, 2400, 0, 0, 0, 0.002f, 1.2f, 0, 0.2f },
    { "Strings", 3, { { W_SAW, 1, -12, 0.35f }, { W_SAW, 1, 0, 0.35f }, { W_SAW, 1, 12, 0.35f } }, 0, 0, 0, 2300, 0, 0,
      1, 0.7f, 0, 1.2f, 0.06f },
    { "Vibraphone", 2, { { W_SINE, 1, 0, 1 }, { W_SINE, 3.99f, 0, 0.18f } }, 0, 0, 0, 0, 0, 0, 0, 0.002f, 3.2f, 0, 0.2f },
};

/* ------------------------------------------------- the FM-1's own: 19..35 --- */
#define NWEB 19
#define SR TP_SR
#define off_after(off, sec) ((off) + (int)((sec) * SR))

/* struck (decaying on its own over d) or held (out over r: for dur seconds, or until let go when dur < 0) */
static void env(tp_voice_t *v, int held, float a, float pk, float dr, float dur)
{
    if (!held) {
        tv_env_ad(v, a, pk, dr);
        tv_len(v, a + dr + 0.1f);
    } else if (dur >= 0.0f) {
        float hold = dur - a > 0.0f ? dur - a : 0.0f;
        tv_env_asr(v, a, pk, hold, dr);
        tv_len(v, a + hold + dr + 0.05f);
    } else
        tv_env_gate(v, a, pk, dr);
}

static float vgain(float vel) { return 0.25f + 0.75f * vel; }

/* -- the drums: a sound a key (the key's place on the keyboard, 0 = F3 .. 26 = G5) -- */
enum { D_KICK, D_KICK2, D_SNARE, D_RIM, D_CLAP, D_BRUSH, D_CHAT, D_PHAT, D_OHAT, D_TOML, D_TOMM, D_TOMH, D_CRASH,
       D_RIDE, D_SHAKER, D_TAMB, D_COWBELL,
       H_DOUM, H_TEK, H_KA, H_FRAME, H_SLAP, H_TAIKO, H_SHIME, H_CLAPPERS, H_UDU, H_FCYM, H_AGOGO_L, H_AGOGO_H, H_RIQ };
static const uint8_t KIT[27] = {
    D_KICK, D_RIM, D_SNARE, D_CLAP, D_TOML, D_CHAT, D_TOMM, D_TOMH, D_PHAT, D_CRASH, D_OHAT, D_RIDE, D_KICK2,
    D_SHAKER, D_BRUSH, D_TAMB, D_COWBELL, D_RIDE, D_KICK, D_RIM, D_SNARE, D_CLAP, D_CHAT, D_OHAT, D_TOML, D_TOMM,
    D_TOMH };
static const uint8_t HAND[27] = {
    H_DOUM, H_TEK, H_KA, H_RIQ, H_FRAME, H_SLAP, H_TAIKO, H_SHIME, H_CLAPPERS, H_UDU, H_FCYM, H_AGOGO_L, H_AGOGO_H,
    D_SHAKER, H_DOUM, H_TEK, H_KA, H_RIQ, H_FRAME, H_SLAP, H_TAIKO, H_SHIME, H_CLAPPERS, H_UDU, H_FCYM, H_AGOGO_L,
    H_AGOGO_H };

static tp_voice_t *tone(int l, int off, int wave, float f, float to, float t, float pk, float d)
{
    tp_voice_t *v = tv_new(l, off);           /* a pitched body, its pitch falling to to over t */
    tv_osc(v, wave, f, 0.0f, 1.0f);
    if (to > 0.0f)
        tv_pitch_ramp(v, to, t);
    tv_env_ad(v, 0.002f, pk, d);
    tv_len(v, d + 0.1f);
    return v;
}

static tp_voice_t *drum(int l, int off, int kind, float g)
{
    tp_voice_t *v = 0;
    int i;
    switch (kind) {
    case D_KICK:
        v = tone(l, off, W_SINE, 150.0f, 45.0f, 0.12f, 0.75f * g, 0.35f);
        tp_burst(l, off, F_BP, 3000.0f, 1.0f, 0.001f, 0.12f * g, 0.012f, 0.03f);
        break;
    case D_KICK2:
        v = tone(l, off, W_SINE, 110.0f, 38.0f, 0.2f, 0.8f * g, 0.6f);
        break;
    case D_SNARE:
        v = tone(l, off, W_TRI, 200.0f, 165.0f, 0.05f, 0.3f * g, 0.12f);
        tp_burst(l, off, F_BP, 3800.0f, 0.8f, 0.001f, 0.5f * g, 0.16f, 0.25f);
        break;
    case D_RIM:
        v = tone(l, off, W_SINE, 1700.0f, 0.0f, 0.0f, 0.25f * g, 0.04f);
        tp_burst(l, off, F_BP, 2600.0f, 4.0f, 0.001f, 0.3f * g, 0.02f, 0.05f);
        break;
    case D_CLAP:
        for (i = 0; i < 3; i++)
            tp_burst(l, off_after(off, (float)i * 0.011f), F_BP, 1500.0f, 1.2f, 0.001f, 0.45f * g, 0.012f, 0.03f);
        tp_burst(l, off_after(off, 0.033f), F_BP, 1400.0f, 1.2f, 0.001f, 0.4f * g, 0.14f, 0.2f);
        break;
    case D_BRUSH:
        v = tv_new(l, off);                   /* a brush swept across the head */
        tv_noise(v, 0, 1.0f);
        tv_filter(v, F_BP, 3200.0f, 0.9f);
        tv_env_ad(v, 0.04f, 0.3f * g, 0.25f);
        tv_len(v, 0.35f);
        break;
    case D_CHAT: tp_burst(l, off, F_HP, 7500.0f, 0.7f, 0.001f, 0.32f * g, 0.04f, 0.08f); break;
    case D_PHAT: tp_burst(l, off, F_HP, 8000.0f, 0.7f, 0.001f, 0.2f * g, 0.025f, 0.05f); break;
    case D_OHAT: tp_burst(l, off, F_HP, 6800.0f, 0.7f, 0.002f, 0.28f * g, 0.38f, 0.45f); break;
    case D_TOML: v = tone(l, off, W_SINE, 105.0f, 75.0f, 0.25f, 0.6f * g, 0.45f); break;
    case D_TOMM: v = tone(l, off, W_SINE, 145.0f, 105.0f, 0.22f, 0.55f * g, 0.38f); break;
    case D_TOMH: v = tone(l, off, W_SINE, 195.0f, 145.0f, 0.2f, 0.5f * g, 0.32f); break;
    case D_CRASH:
        v = tv_new(l, off);
        tv_noise(v, 0, 1.0f);
        tv_filter(v, F_HP, 4200.0f, 0.7f);
        tv_env_ad(v, 0.002f, 0.3f * g, 1.6f);
        tv_len(v, 1.7f);
        break;
    case D_RIDE:
        v = tv_new(l, off);                   /* the bell of the cymbal, and its wash */
        tv_osc(v, W_SINE, 3150.0f, 0.0f, 1.0f);
        tv_fm(v, 1.47f, 2400.0f, 0.4f);
        tv_env_ad(v, 0.001f, 0.06f * g, 0.9f);
        tv_len(v, 1.0f);
        tp_burst(l, off, F_HP, 8000.0f, 0.7f, 0.001f, 0.12f * g, 0.5f, 0.6f);
        break;
    case D_SHAKER: tp_burst(l, off, F_BP, 6000.0f, 1.0f, 0.012f, 0.3f * g, 0.06f, 0.1f); break;
    case D_TAMB:
        for (i = 0; i < 2; i++)
            tp_burst(l, off_after(off, (float)i * 0.02f), F_BP, 7400.0f, 3.0f, 0.001f, 0.3f * g, 0.12f, 0.16f);
        break;
    case D_COWBELL:
        v = tv_new(l, off);
        tv_osc(v, W_SQR, 545.0f, 0.0f, 0.6f);
        tv_osc(v, W_SQR, 815.0f, 0.0f, 0.6f);
        tv_filter(v, F_BP, 900.0f, 2.0f);
        tv_env_ad(v, 0.001f, 0.35f * g, 0.35f);
        tv_len(v, 0.45f);
        break;
    case H_DOUM:                              /* the darbuka: its deep centre */
        v = tone(l, off, W_SINE, 95.0f, 68.0f, 0.15f, 0.65f * g, 0.4f);
        tp_burst(l, off, F_LP, 600.0f, 0.7f, 0.002f, 0.12f * g, 0.05f, 0.1f);
        break;
    case H_TEK:                               /* its rim, bright */
        v = tone(l, off, W_SINE, 880.0f, 0.0f, 0.0f, 0.18f * g, 0.06f);
        tp_burst(l, off, F_BP, 3300.0f, 3.0f, 0.001f, 0.35f * g, 0.04f, 0.08f);
        break;
    case H_KA: tp_burst(l, off, F_BP, 2000.0f, 1.5f, 0.001f, 0.4f * g, 0.035f, 0.07f); break;
    case H_FRAME:                             /* a frame drum: low and boomy */
        v = tone(l, off, W_SINE, 78.0f, 60.0f, 0.3f, 0.6f * g, 0.65f);
        tp_burst(l, off, F_LP, 400.0f, 0.7f, 0.003f, 0.15f * g, 0.12f, 0.2f);
        break;
    case H_SLAP: tp_burst(l, off, F_BP, 1200.0f, 1.2f, 0.001f, 0.4f * g, 0.06f, 0.1f); break;
    case H_TAIKO:
        v = tone(l, off, W_SINE, 95.0f, 55.0f, 0.25f, 0.7f * g, 0.85f);
        tp_burst(l, off, F_LP, 700.0f, 0.7f, 0.002f, 0.18f * g, 0.1f, 0.2f);
        break;
    case H_SHIME:                             /* a small, tight taiko */
        v = tone(l, off, W_SINE, 310.0f, 250.0f, 0.06f, 0.4f * g, 0.16f);
        tp_burst(l, off, F_BP, 1800.0f, 2.0f, 0.001f, 0.2f * g, 0.03f, 0.06f);
        break;
    case H_CLAPPERS:
        v = tv_new(l, off);
        tv_osc(v, W_SINE, 2100.0f, 0.0f, 1.0f);
        tv_osc(v, W_SINE, 3400.0f, 0.0f, 0.4f);
        tv_env_ad(v, 0.001f, 0.2f * g, 0.08f);
        tv_len(v, 0.12f);
        tp_burst(l, off, F_BP, 2600.0f, 3.0f, 0.001f, 0.2f * g, 0.02f, 0.05f);
        break;
    case H_UDU:                               /* the clay pot's bass: its pitch rising, a hollow bloop */
        v = tone(l, off, W_SINE, 120.0f, 175.0f, 0.12f, 0.55f * g, 0.4f);
        break;
    case H_FCYM:
        v = tv_new(l, off);                   /* finger cymbals: three high partials ringing */
        tv_osc(v, W_SINE, 3420.0f, 0.0f, 1.0f);
        tv_osc(v, W_SINE, 5130.0f, 0.0f, 0.6f);
        tv_osc(v, W_SINE, 6890.0f, 0.0f, 0.4f);
        tv_env_ad(v, 0.001f, 0.08f * g, 2.0f);
        tv_len(v, 2.1f);
        break;
    case H_AGOGO_L:
    case H_AGOGO_H: {
        float f = kind == H_AGOGO_L ? 880.0f : 1320.0f;
        v = tv_new(l, off);
        tv_osc(v, W_SINE, f, 0.0f, 1.0f);
        tv_osc(v, W_SINE, f * 2.4f, 0.0f, 0.3f);
        tv_env_ad(v, 0.001f, 0.18f * g, 0.45f);
        tv_len(v, 0.55f);
        break;
    }
    case H_RIQ:                               /* the tambourine's jingles, shaken */
        for (i = 0; i < 3; i++)
            tp_burst(l, off_after(off, (float)i * 0.03f), F_BP, 6800.0f, 6.0f, 0.001f, (0.4f - (float)i * 0.08f) * g,
                     0.07f, 0.1f);
        break;
    }
    return v;
}

static int key_of(int midi) { int k = midi - 53; return k < 0 ? 0 : k > 26 ? 26 : k; }
static tp_voice_t *drum_kit(int l, int off, float f, int midi, float vel, float dur)
{
    return drum(l, off, KIT[key_of(midi)], 0.6f * vgain(vel));
}
static tp_voice_t *hand_drums(int l, int off, float f, int midi, float vel, float dur)
{
    return drum(l, off, HAND[key_of(midi)], 0.6f * vgain(vel));
}

/* -- the instruments -- */
static tp_voice_t *rhodes(int l, int off, float f, int midi, float vel, float dur)
{
    tp_voice_t *v = tv_new(l, off);           /* a struck tine: its bark when hit hard, a gentle tremolo */
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 2.0f, 0.0f, 0.15f);
    tv_fm(v, 1.0f, f * (0.3f + 1.2f * vel), 0.35f);
    tv_amp(v, 0.9f);
    tv_lfo(v, LFO_AMP, 4.5f, 0.1f, 0);
    env(v, 0, 0.002f, 0.17f * vgain(vel), 2.4f, dur);
    return v;
}

static tp_voice_t *harpsichord(int l, int off, float f, int midi, float vel, float dur)
{
    tp_voice_t *v = tv_new(l, off);
    tv_osc(v, W_SAW, f, 0.0f, 1.0f);
    tv_osc(v, W_SQR, f * 2.0f, 0.0f, 0.3f);
    tv_filter(v, F_LP, 5200.0f, 1.0f);
    tv_filter_ramp(v, 1, 1500.0f, 0.25f);
    env(v, 0, 0.001f, 0.24f * vgain(vel), 1.1f, dur);
    return v;
}

static tp_voice_t *celesta(int l, int off, float f, int midi, float vel, float dur)
{
    tp_voice_t *v = tv_new(l, off);           /* hammers on steel bars in a wooden box: soft, bell-like */
    tv_osc(v, W_SINE, f * 2.0f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 4.0f, 0.0f, 0.12f);
    tv_fm(v, 3.0f, f * 0.8f, 0.08f);
    env(v, 0, 0.001f, 0.12f * vgain(vel), 1.5f, dur);
    return v;
}

static tp_voice_t *toy_piano(int l, int off, float f, int midi, float vel, float dur)
{
    tp_voice_t *v = tv_new(l, off);           /* a struck rod: out-of-tune partials, a short ring */
    tv_osc(v, W_SINE, f * 2.0f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 6.2f, 0.0f, 0.4f);
    tv_osc(v, W_SINE, f * 10.8f, 0.0f, 0.15f);
    env(v, 0, 0.001f, 0.14f * vgain(vel), 0.75f, dur);
    return v;
}

static tp_voice_t *koto(int l, int off, float f, int midi, float vel, float dur)
{
    tp_voice_t *v = tv_new(l, off);           /* a silk string: bright, a little bend down */
    tv_osc(v, W_SAW, f * 1.01f, 0.0f, 0.6f);
    tv_osc(v, W_TRI, f * 1.01f, 0.0f, 1.0f);
    tv_pitch_ramp(v, f, 0.08f);
    tv_filter(v, F_LP, 2500.0f + 2500.0f * vel, 2.0f);
    tv_filter_ramp(v, 1, 900.0f, 0.3f);
    env(v, 0, 0.002f, 0.1f * vgain(vel), 1.4f, dur);
    return v;
}

static tp_voice_t *oud(int l, int off, float f, int midi, float vel, float dur)
{
    tp_voice_t *v = tv_new(l, off);           /* a fretless lute: a woody pluck settling onto the note */
    tv_osc(v, W_TRI, f * 1.015f, 0.0f, 1.0f);
    tv_osc(v, W_SAW, f * 1.015f, 4.0f, 0.4f);
    tv_pitch_ramp(v, f, 0.05f);
    tv_filter(v, F_LP, 1400.0f + 1400.0f * vel, 1.5f);
    tv_filter_ramp(v, 1, 600.0f, 0.4f);
    env(v, 0, 0.002f, 0.13f * vgain(vel), 1.0f, dur);
    tp_burst(l, off, F_BP, 2400.0f, 2.0f, 0.001f, 0.06f * vgain(vel), 0.02f, 0.04f);
    return v;
}

static tp_voice_t *shakuhachi(int l, int off, float f, int midi, float vel, float dur)
{
    tp_voice_t *v = tv_new(l, off);           /* breath first, the note bending up into place, a slow vibrato */
    tv_osc(v, W_SINE, f * 0.95f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 1.9f, 0.0f, 0.1f);
    tv_pitch_ramp(v, f, 0.3f);
    tv_lfo(v, LFO_DET, 4.0f, 11.0f, 3);
    env(v, 1, 0.2f, 0.18f * vgain(vel), 0.5f, dur);
    tp_burst(l, off, F_BP, 1600.0f, 0.7f, 0.12f, 0.05f * vgain(vel), 0.4f, 0.6f);
    return v;
}

static tp_voice_t *steel_pan(int l, int off, float f, int midi, float vel, float dur)
{
    tp_voice_t *v = tv_new(l, off);           /* a tuned steel dish: the note, its octave and twelfth, ringing */
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 2.01f, 0.0f, 0.5f);
    tv_osc(v, W_SINE, f * 3.0f, 0.0f, 0.2f);
    tv_fm(v, 2.0f, f * 0.5f * vel, 0.06f);
    env(v, 0, 0.002f, 0.16f * vgain(vel), 1.3f, dur);
    return v;
}

static tp_voice_t *violin(int l, int off, float f, int midi, float vel, float dur)
{
    tp_voice_t *v = tv_new(l, off);           /* a slide into the note, a wide vibrato */
    tv_osc(v, W_SAW, f * 0.97f, 0.0f, 1.0f);
    tv_osc(v, W_SAW, f * 0.97f, 5.0f, 0.4f);
    tv_pitch_ramp(v, f, 0.12f);
    tv_lfo(v, LFO_DET, 6.0f, 18.0f, 3);
    tv_filter(v, F_LP, 2000.0f + 1400.0f * vel, 1.0f);
    env(v, 1, 0.12f, 0.07f * vgain(vel), 0.35f, dur);
    return v;
}

static tp_voice_t *muted_trumpet(int l, int off, float f, int midi, float vel, float dur)
{
    tp_voice_t *v = tv_new(l, off);           /* a mute in the bell: nasal, a little scoop up */
    tv_osc(v, W_SAW, f * 0.97f, 0.0f, 1.0f);
    tv_osc(v, W_SQR, f * 0.97f, 0.0f, 0.3f);
    tv_pitch_ramp(v, f, 0.06f);
    tv_filter(v, F_BP, 1300.0f, 1.5f);
    tv_filter(v, F_LP, 3500.0f, 0.7f);
    env(v, 1, 0.04f, 0.2f * vgain(vel), 0.2f, dur);
    return v;
}

static tp_voice_t *soprano(int l, int off, float f, int midi, float vel, float dur)
{
    tp_voice_t *v = tv_new(l, off);           /* a wordless "ah": two formants side by side */
    tv_osc(v, W_SAW, f, 0.0f, 1.0f);
    tv_osc(v, W_TRI, f, 0.0f, 0.8f);
    tv_lfo(v, LFO_DET, 5.4f, 18.0f, 3);
    tv_filter(v, F_BP, 820.0f, 5.0f);
    tv_filter(v, F_BP, 1250.0f, 6.0f);
    tv_filter_parallel(v);
    env(v, 1, 0.25f, 0.24f * vgain(vel), 0.8f, dur);
    return v;
}

static tp_voice_t *synth_lead(int l, int off, float f, int midi, float vel, float dur)
{
    tp_voice_t *v = tv_new(l, off);           /* two saws and a sub, gliding into the note */
    tv_osc(v, W_SAW, f * 0.98f, -6.0f, 1.0f);
    tv_osc(v, W_SAW, f * 0.98f, 6.0f, 1.0f);
    tv_osc(v, W_SQR, f * 0.49f, 0.0f, 0.4f);
    tv_pitch_ramp(v, f, 0.05f);
    tv_filter(v, F_LP, 1500.0f + 2000.0f * vel, 4.0f);
    env(v, 1, 0.01f, 0.06f * vgain(vel), 0.22f, dur);
    return v;
}

static tp_voice_t *upright_bass(int l, int off, float f, int midi, float vel, float dur)
{
    tp_voice_t *v = tv_new(l, off);           /* a plucked double bass, an octave below the key */
    f *= 0.5f;
    tv_osc(v, W_TRI, f * 1.01f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 1.01f, 0.0f, 0.6f);
    tv_osc(v, W_SAW, f * 1.01f, 0.0f, 0.15f);
    tv_pitch_ramp(v, f, 0.04f);
    tv_filter(v, F_LP, 700.0f + 500.0f * vel, 1.0f);
    tv_filter_ramp(v, 1, 300.0f, 0.4f);
    env(v, 0, 0.003f, 0.35f * vgain(vel), 1.1f, dur);
    tp_burst(l, off, F_LP, 1200.0f, 0.7f, 0.001f, 0.08f * vgain(vel), 0.02f, 0.04f);
    return v;
}

static tp_voice_t *temple_bell(int l, int off, float f, int midi, float vel, float dur)
{
    tp_voice_t *v = tv_new(l, off);           /* a great bell: its hum an octave down, a long ring */
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 0.5f, 0.0f, 0.5f);
    tv_fm(v, 1.4f, f * (0.6f + vel), 2.5f);
    env(v, 0, 0.003f, 0.12f * vgain(vel), 6.0f, dur);
    return v;
}

static tp_voice_t *singing_bowl(int l, int off, float f, int midi, float vel, float dur)
{
    tp_voice_t *v = tv_new(l, off);           /* rubbed, then ringing: two partials beating */
    tv_osc(v, W_SINE, f, 0.0f, 1.0f);
    tv_osc(v, W_SINE, f * 2.71f, 0.0f, 0.35f);
    tv_osc(v, W_SINE, f * 2.71f, 7.0f, 0.35f);
    tv_amp(v, 0.85f);
    tv_lfo(v, LFO_AMP, 1.7f, 0.15f, 0);
    env(v, 0, 0.3f, 0.12f * vgain(vel), 6.0f, dur);
    return v;
}

typedef tp_voice_t *(*instr_fn)(int, int, float, int, float, float);
static const struct { const char *name; instr_fn fn; } OWN[TP_NINSTR - NWEB] = {
    { "Drum kit", drum_kit },         { "Hand drums", hand_drums },     { "Rhodes", rhodes },
    { "Harpsichord", harpsichord },   { "Celesta", celesta },           { "Toy piano", toy_piano },
    { "Koto", koto },                 { "Oud", oud },                   { "Shakuhachi", shakuhachi },
    { "Steel pan", steel_pan },       { "Violin", violin },             { "Muted trumpet", muted_trumpet },
    { "Soprano", soprano },           { "Synth lead", synth_lead },     { "Upright bass", upright_bass },
    { "Temple bell", temple_bell },   { "Singing bowl", singing_bowl },
};

int tp_instr_drums(int instr) { return instr == NWEB || instr == NWEB + 1; }

const char *tp_instr_name(int instr)
{
    if (instr >= NWEB && instr < TP_NINSTR)
        return OWN[instr - NWEB].name;
    return INSTR[instr < 0 || instr >= NWEB ? 0 : instr].name;
}

/* voiceNote: struck sounds decay on their own (a held key does not hold them); held ones sustain for dur (a
 * recorded note) or until released (a key). The loop's attack and decay tweaks apply in tv_env_* */
tp_voice_t *tp_instr_note(int loop, int off, int instr, int midi, float vel, float dur)
{
    const instr_t *V;
    float f = tp_mtof((float)midi), pk;
    int k;
    tp_voice_t *v;
    if (instr >= NWEB && instr < TP_NINSTR)
        return OWN[instr - NWEB].fn(loop, off, f, midi, vel, dur);
    V = &INSTR[instr < 0 || instr >= NWEB ? 0 : instr];
    pk = V->gain * (0.25f + 0.75f * vel);
    v = tv_new(loop, off);
    if (!v)
        return 0;
    for (k = 0; k < V->nw; k++)
        tv_osc(v, V->w[k].wave, f * V->w[k].ratio, V->w[k].det, V->w[k].lvl);
    if (V->fm_r > 0.0f)
        tv_fm(v, V->fm_r, f * V->fm_i * (0.5f + vel), V->fm_dec);
    if (V->lp > 0.0f)
        tv_filter(v, F_LP, V->lp * (0.6f + 0.4f * vel), 0.7f);
    else if (V->bp_f > 0.0f)
        tv_filter(v, F_BP, V->bp_f, V->bp_q);
    if (!V->sus) {                               /* struck: envAD, stopped at a + d + 0.1 */
        tv_env_ad(v, V->a, pk, V->d);
        tv_len(v, V->a + V->d + 0.1f);
    } else if (dur >= 0.0f) {                    /* held for dur: envASR, its hold what the attack leaves */
        float hold = dur - V->a;
        tv_env_asr(v, V->a, pk, hold > 0.0f ? hold : 0.0f, V->r);
        tv_len(v, V->a + (hold > 0.0f ? hold : 0.0f) + V->r + 0.05f);
    } else                                       /* a key held: until it is let go */
        tv_env_gate(v, V->a, pk, V->r);
    return v;
}
