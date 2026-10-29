/* Tidepool on the FM-1: the engine (see tp_engine.h). */
#include "tp_engine.h"
#include "tp_voice.h"
#include "tp_canvas.h"
#include "tp_math.h"
#if __has_include(<string.h>)
#include <string.h>
#else                                  /* an fm1-base firmware: no C library headers; its libc.c has memset */
#include <stddef.h>
void *memset(void *dst, int c, size_t n);
#endif

#define INV_SR (1.0f / TP_SR)
#define ENV_FLOOR 1e-4f                /* where Web Audio's exponential ramps end */

/* ------------------------------------------------------------------ voices --- */
enum { E_AD, E_ASR, E_CONST, E_GATE };
enum { ST_ATT, ST_DEC, ST_FLOOR, ST_HOLD, ST_REL, ST_CONST, ST_DONE, ST_SUS };

struct tp_voice {
    uint8_t on, loop, nparts, persist, etype, stage, noise, fm, nlfo, shed;
    uint8_t ftype[2], wave[TP_NPART];
    float ph[TP_NPART], inc[TP_NPART], base_inc[TP_NPART], lvl[TP_NPART], pdec[TP_NPART];
    float fm_ph, fm_inc, fm_idx, fm_mul;
    int32_t fm_n;
    float pr_mul;
    int32_t pr_n;
    float nlvl, brown;
    uint32_t nr;                                /* the note's noise generator */
    float bfc, k[2], a1[2], a2[2], a3[2], s1[2], s2[2];
    uint8_t fpar, nseg, seg;                    /* the two filters side by side (formants); filter 0's sweep stages */
    struct { uint8_t exp; float to; int32_t n; } fseg[3];
    float fc_now, fstep;                        /* the sweep's cutoff now, and its per-block step (exp: a ratio) */
    int32_t fleft;
    float amp, amp_base;                        /* a gain after the envelope: a chord's tremolo (LFO_AMP) */
    struct { uint8_t tgt, mask; float ph, inc, depth; } lfo[4];
    float gain, env, peak, ainc, dmul, dmul32, rdec;
    int32_t n, hold, rel, wait, left, ext;
    uint32_t born;
    uint16_t nid;                               /* a held note's id (tp_live_on), for its release */
};

static tp_voice_t vs[TP_MAXVOICE] TP_BIG;
/* each loop's envelope tweaks, as the web app's: attack 0..1 (0 = the sound's own, else at least at^2 * 2.5 s),
 * decay 0..1 (0.5 = its own; times 2^((de - 0.5) * 4), so 1/4x .. 4x, on decays and releases) */
static float lt_at[TP_NBUS], lt_de[TP_NBUS];
static inline float tweak_a(int l, float a)
{
    float t = lt_at[l] * lt_at[l] * 2.5f;
    return lt_at[l] > 0.0f && t > a ? t : a;
}
static inline float tweak_df(int l) { return tp_exp2f((lt_de[l] - 0.5f) * 4.0f); }
#define SIN_N 1024
static float sintab[SIN_N + 1] TP_BIG;       /* one cycle, linear interpolation: 4 KB, error ~1e-6 */
static inline float sin_tab(float ph)
{
    float x = ph * (float)SIN_N;
    int32_t i = (int32_t)x;
    float f = x - (float)i;
    return sintab[i] + f * (sintab[i + 1] - sintab[i]);
}
static int vlimit = TP_MAXVOICE;
static uint32_t born_ctr, rng = 0x12345678u;
tp_stats_t tp_stats;

static inline uint32_t xs32(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}
float tp_rand(void) { return (float)(xs32() >> 8) * (1.0f / 16777216.0f); }
/* each note's noise comes from its own generator, seeded as the note starts: how long one note sounds never changes
 * another's noise, nor the loops' random choices (rng) */
static inline float noise_bi(uint32_t *r)
{
    uint32_t x = *r;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *r = x;
    return (float)(int32_t)x * (1.0f / 2147483648.0f);
}

static void svf_set(tp_voice_t *v, int j, float fc)
{
    float g;
    if (fc < 10.0f)
        fc = 10.0f;
    if (fc > 0.45f * TP_SR)
        fc = 0.45f * TP_SR;
    g = tp_tan_pi(fc * INV_SR);
    v->a1[j] = 1.0f / (1.0f + g * (g + v->k[j]));
    v->a2[j] = g * v->a1[j];
    v->a3[j] = g * v->a2[j];
}

tp_voice_t *tv_new(int loop, int off)
{
    tp_voice_t *v = 0, *old = 0;
    int i, n = 0;
    for (i = 0; i < TP_MAXVOICE; i++) {
        if (vs[i].on) {
            n++;
            if (!vs[i].persist && (!old || vs[i].born < old->born))
                old = &vs[i];
        } else if (!v)
            v = &vs[i];
    }
    if (n >= vlimit || !v) {
        if (!old) {
            tp_stats.dropped++;
            return 0;
        }
        v = old;                       /* the oldest note gives way (a hard cut; the device would fade it) */
        tp_stats.stolen++;
    }
    memset(v, 0, sizeof *v);
    v->on = 1;
    v->loop = (uint8_t)loop;
    v->wait = off;
    v->left = 0x7FFFFFFF;
    v->born = ++born_ctr;
    v->nr = (born_ctr * 2654435761u) ^ 0x9E3779B9u;
    if (!v->nr)
        v->nr = 1u;
    v->amp = v->amp_base = 1.0f;
    for (i = 0; i < TP_NPART; i++)
        v->pdec[i] = 1.0f;
    tp_stats.started++;
    return v;
}

void tv_osc(tp_voice_t *v, int wave, float hz, float cents, float level)
{
    int p;
    if (!v || v->nparts >= TP_NPART)
        return;
    p = v->nparts++;
    v->wave[p] = (uint8_t)wave;
    v->base_inc[p] = v->inc[p] = hz * (cents != 0.0f ? tp_cents(cents) : 1.0f) * INV_SR;
    /* Chromium scales its band-limited sawtooth and square down to peak at 1 (their Gibbs overshoot): 1.45 dB
     * below the ideal waves (measured: OfflineAudioContext, 55..880 Hz). Sine and triangle are exact. */
    v->lvl[p] = (wave == W_SAW || wave == W_SQR) ? level * 0.8462f : level;
    /* and every wave starts as Chromium's does, at zero rising, in step with a sine (its Fourier series are sines):
     * a triangle and a sine on the same note then add as in the browser, not a quarter cycle apart */
    v->ph[p] = wave == W_SAW ? 0.5f : wave == W_TRI ? 0.25f : 0.0f;
}

/* a partial with its own, faster exponential decay under the voice's AD envelope */
void tv_part_decay(tp_voice_t *v, int part, float peak, float d, float env_peak, float env_d)
{
    float own, env;
    if (!v)
        return;
    own = tp_powf(ENV_FLOOR / peak, 1.0f / (d * tweak_df(v->loop) * TP_SR));
    env = v->etype == E_AD ? v->dmul : tp_powf(ENV_FLOOR / env_peak, 1.0f / (env_d * TP_SR));
    v->pdec[part] = own / env;
    v->lvl[part] *= peak / env_peak;
}

void tv_noise(tp_voice_t *v, int brown, float level)
{
    if (!v)
        return;
    v->noise = brown ? 2 : 1;
    v->nlvl = level;
}

void tv_filter(tp_voice_t *v, int type, float hz, float q)
{
    int j;
    if (!v)
        return;
    j = v->ftype[0] ? 1 : 0;
    v->ftype[j] = (uint8_t)type;
    v->k[j] = 1.0f / (type == F_BP ? q : tp_db2lin(q));     /* Web Audio: LP / HP Q is in dB */
    if (j == 0)
        v->bfc = v->fc_now = hz;
    svf_set(v, j, hz);
}

/* filter 0's cutoff moves to `to` over `seconds` after the previous stage: Web Audio's linearRampToValueAtTime
 * (exp 0) or exponentialRampToValueAtTime (exp 1), at control rate */
void tv_filter_ramp(tp_voice_t *v, int exp, float to, float seconds)
{
    if (!v || v->nseg >= 3)
        return;
    v->fseg[v->nseg].exp = (uint8_t)exp;
    v->fseg[v->nseg].to = to;
    v->fseg[v->nseg].n = (int32_t)(seconds * TP_SR);
    v->nseg++;
}

void tv_filter_parallel(tp_voice_t *v)
{
    if (v)
        v->fpar = 1;
}

void tv_amp(tp_voice_t *v, float gain)
{
    if (v)
        v->amp = v->amp_base = gain;
}

void tv_env_ad(tp_voice_t *v, float a, float peak, float d)
{
    float a2, d2;
    if (!v)
        return;
    a2 = tweak_a(v->loop, a);
    d2 = d * tweak_df(v->loop);
    v->ext = (int32_t)((a2 - a + d2 - d) * TP_SR);
    a = a2;
    d = d2;
    v->etype = E_AD;
    v->stage = ST_ATT;
    v->peak = peak;
    v->n = (int32_t)(a * TP_SR);
    if (v->n < 1)
        v->n = 1;
    v->ainc = peak / (float)v->n;
    v->hold = (int32_t)(d * TP_SR);
    if (v->hold < 1)
        v->hold = 1;
    v->dmul = tp_powf(ENV_FLOOR / peak, 1.0f / (float)v->hold);
    v->dmul32 = v->dmul * v->dmul;           /* a whole block's decay: dmul^32 */
    v->dmul32 *= v->dmul32;
    v->dmul32 *= v->dmul32;
    v->dmul32 *= v->dmul32;
    v->dmul32 *= v->dmul32;
}

void tv_env_asr(tp_voice_t *v, float a, float peak, float hold, float r)
{
    float a2, h2, r2;
    if (!v)
        return;
    a2 = tweak_a(v->loop, a);              /* a slower swell eats into the hold: the note keeps its length */
    h2 = hold - (a2 - a);
    if (h2 < 0.0f)
        h2 = 0.0f;
    r2 = r * tweak_df(v->loop);
    v->ext = (int32_t)(((a2 + (h2 > 0.0f ? h2 : 0.0f) + r2) - (a + (hold > 0.0f ? hold : 0.0f) + r)) * TP_SR);
    a = a2;
    hold = h2;
    r = r2;
    v->etype = E_ASR;
    v->stage = ST_ATT;
    v->peak = peak;
    v->n = (int32_t)(a * TP_SR);
    if (v->n < 1)
        v->n = 1;
    v->ainc = peak / (float)v->n;
    v->hold = hold > 0.0f ? (int32_t)(hold * TP_SR) : 0;
    v->rel = (int32_t)(r * TP_SR);
    if (v->rel < 1)
        v->rel = 1;
    v->rdec = peak / (float)v->rel;
}

/* a held note: in over a (the loop's attack tweak), held until tp_live_off, out over r (its decay tweak) */
void tv_env_gate(tp_voice_t *v, float a, float peak, float r)
{
    if (!v)
        return;
    a = tweak_a(v->loop, a);
    r *= tweak_df(v->loop);
    v->etype = E_GATE;
    v->stage = ST_ATT;
    v->peak = peak;
    v->n = (int32_t)(a * TP_SR);
    if (v->n < 1)
        v->n = 1;
    v->ainc = peak / (float)v->n;
    v->rel = (int32_t)(r * TP_SR);
    if (v->rel < 1)
        v->rel = 1;
}

void tv_set_id(tp_voice_t *v, uint16_t id)
{
    if (v)
        v->nid = id;
}

void tv_env_const(tp_voice_t *v, float gain)
{
    if (!v)
        return;
    v->etype = E_CONST;
    v->stage = ST_CONST;
    v->env = v->gain = gain;
}

void tv_len(tp_voice_t *v, float seconds)
{
    if (v)
        v->left = (int32_t)(seconds * TP_SR) + (v->ext > 0 ? v->ext : 0);
}

void tv_pitch_ramp(tp_voice_t *v, float to_hz, float t)
{
    float from;
    if (!v || !v->nparts)
        return;
    from = v->base_inc[0] * TP_SR;
    v->pr_n = (int32_t)(t * TP_SR);
    v->pr_mul = tp_powf(to_hz / from, 1.0f / (float)v->pr_n);
}

void tv_fm(tp_voice_t *v, float ratio, float index_hz, float decay)
{
    if (!v || !v->nparts)
        return;
    v->fm = 1;
    v->fm_inc = v->base_inc[0] * ratio;
    v->fm_idx = index_hz;
    v->fm_n = (int32_t)(decay * TP_SR);
    v->fm_mul = tp_powf(1.0f / index_hz, 1.0f / (float)v->fm_n);
}

void tv_lfo(tp_voice_t *v, int target, float rate, float depth, int part_mask)
{
    if (!v || v->nlfo >= 4)
        return;
    v->lfo[v->nlfo].tgt = (uint8_t)target;
    v->lfo[v->nlfo].mask = (uint8_t)part_mask;
    v->lfo[v->nlfo].ph = 0.0f;
    v->lfo[v->nlfo].inc = rate * INV_SR;
    v->lfo[v->nlfo].depth = depth;
    v->nlfo++;
}

void tv_persist(tp_voice_t *v)
{
    if (v)
        v->persist = 1;
}

void tp_burst(int loop, int off, int type, float f, float q, float a, float peak, float d, float len)
{
    tp_voice_t *v = tv_new(loop, off);
    tv_noise(v, 0, 1.0f);
    tv_filter(v, type, f, q);
    tv_env_ad(v, a, peak, d);
    tv_len(v, len);
}

/* ------------------------------------------------------------ voice render --- */
/* the same with 1/dt given (no division in the block's loop) */
static inline float blep_r(float t, float dt, float rdt)
{
    if (t < dt) {
        t *= rdt;
        return t + t - t * t - 1.0f;
    }
    if (t > 1.0f - dt) {
        t = (t - 1.0f) * rdt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

static void sweep_step(tp_voice_t *v)        /* one block of filter 0's sweep */
{
    while (v->seg < v->nseg) {
        if (v->fleft <= 0) {                  /* a stage begins */
            int32_t n = v->fseg[v->seg].n < 1 ? 1 : v->fseg[v->seg].n;
            float to = v->fseg[v->seg].to;
            v->fleft = n;
            v->fstep = v->fseg[v->seg].exp ? tp_powf(to / v->fc_now, (float)TP_BLOCK / (float)n)
                                           : (to - v->fc_now) * (float)TP_BLOCK / (float)n;
        }
        if (v->fseg[v->seg].exp)
            v->fc_now *= v->fstep;
        else
            v->fc_now += v->fstep;
        if ((v->fleft -= TP_BLOCK) <= 0) {    /* the stage is done: exactly where it was going */
            v->fc_now = v->fseg[v->seg].to;
            v->seg++;
            v->fleft = 0;
        }
        return;
    }
}

static void lfo_update(tp_voice_t *v)
{
    float fc = 0.0f, gain = 0.0f, amp = 0.0f;
    int i, p, has_fc = v->nseg > 0, has_amp = 0;
    if (v->nseg)
        sweep_step(v);
    for (i = 0; i < v->nlfo; i++) {
        float val = v->lfo[i].depth * tp_sin_turns(v->lfo[i].ph);
        v->lfo[i].ph += v->lfo[i].inc * (float)TP_BLOCK;
        if (v->lfo[i].ph >= 1.0f)
            v->lfo[i].ph -= 1.0f;
        if (v->lfo[i].tgt == LFO_FC) {
            fc += val;
            has_fc = 1;
        } else if (v->lfo[i].tgt == LFO_GAIN)
            gain += val;
        else if (v->lfo[i].tgt == LFO_AMP) {
            amp += val;
            has_amp = 1;
        } else
            for (p = 0; p < v->nparts; p++)
                if (v->lfo[i].mask & (1 << p))
                    v->inc[p] = v->base_inc[p] * tp_cents(val);
    }
    if (has_fc)
        svf_set(v, 0, (v->nseg ? v->fc_now : v->bfc) + fc);
    if (has_amp)
        v->amp = v->amp_base + amp;
    if (v->etype == E_CONST)
        v->env = v->gain + gain;
}

/* the envelope over n samples, at control rate: where it ends; the block interpolates it linearly in between (an
 * exponential decay over 0.7 ms is a straight line to within a fraction of a dB) */
static float env_advance(tp_voice_t *v, int n)
{
    while (n > 0) {
        int k;
        switch (v->stage) {
        case ST_ATT:
            k = n < v->n ? n : v->n;
            v->env += v->ainc * (float)k;
            n -= k;
            if ((v->n -= k) <= 0) {
                v->env = v->peak;
                v->stage = v->etype == E_AD ? ST_DEC : v->etype == E_GATE ? ST_SUS : ST_HOLD;
                v->n = v->hold;
            }
            break;
        case ST_DEC:
            k = n < v->n ? n : v->n;
            if (k == TP_BLOCK)
                v->env *= v->dmul32;
            else {
                int j;
                for (j = 0; j < k; j++)
                    v->env *= v->dmul;
            }
            n -= k;
            if ((v->n -= k) <= 0) {
#ifdef TP_HOLD_FLOOR                         /* (the web app's way, for comparison: hold -80 dB until the stop) */
                v->env = ENV_FLOOR;
                v->stage = ST_FLOOR;
#else                                        /* at -80 dB the note ends (the block fades the last step to zero),
                                              * and its time is free; the web app holds it until its stop */
                v->env = 0.0f;
                v->stage = ST_DONE;
                n = 0;
#endif
            }
            break;
        case ST_HOLD:
            k = n < v->n ? n : v->n;
            n -= k;
            if ((v->n -= k) <= 0) {
                v->stage = ST_REL;
                v->n = v->rel;
            }
            break;
        case ST_REL:
            k = n < v->n ? n : v->n;
            v->env -= v->rdec * (float)k;
            n -= k;
            if ((v->n -= k) <= 0) {
                v->env = 0.0f;
                v->stage = ST_DONE;
            }
            break;
        default:                             /* the floor, a constant sound, done: no change */
            n = 0;
        }
    }
    return v->env;
}

/* one filter over a block, in place */
static void svf_run(tp_voice_t *v, int p, float *x, int n)
{
    float s1 = v->s1[p], s2 = v->s2[p], a1 = v->a1[p], a2 = v->a2[p], a3 = v->a3[p], k = v->k[p];
    int t = v->ftype[p], i;
    for (i = 0; i < n; i++) {
        float v3 = x[i] - s2, v1 = a1 * s1 + a2 * v3, v2 = s2 + a2 * s1 + a3 * v3;
        s1 = 2.0f * v1 - s1;
        s2 = 2.0f * v2 - s2;
        x[i] = t == F_LP ? v2 : t == F_BP ? k * v1 : x[i] - k * v1 - v2;
    }
    v->s1[p] = s1;
    v->s2[p] = s2;
}

/* One block of a voice: each oscillator across the block (its wave chosen once, not every sample),
 * then the noise, the filters and the envelope, each a tight loop over the block. */
static void voice_block(tp_voice_t *v, float *bus)
{
    float x[TP_BLOCK];
    int i, p, i0, n, last = 0;
    if (v->wait >= TP_BLOCK) {
        v->wait -= TP_BLOCK;
        return;
    }
    i0 = v->wait;
    v->wait = 0;
    n = TP_BLOCK - i0;
    if (v->left <= n) {
        n = v->left;
        last = 1;
    }
    v->left -= n;
    if (v->nlfo || v->nseg)
        lfo_update(v);
    for (i = 0; i < n; i++)
        x[i] = 0.0f;

    for (p = 0; p < v->nparts; p++) {
        float ph = v->ph[p], inc = v->inc[p], inc0 = inc, lvl = v->lvl[p], pd = v->pdec[p];
        float pm = v->pr_n > 0 ? v->pr_mul : 1.0f;      /* a pitch ramp: the increment glides */
        switch (v->wave[p]) {
        case W_SINE:
            if (p == 0 && v->fm) {     /* the modulator's output, in Hz, added to the carrier's frequency */
                float mph = v->fm_ph, minc = v->fm_inc, idx = v->fm_idx, im = v->fm_n > 0 ? v->fm_mul : 1.0f;
                for (i = 0; i < n; i++) {
                    float ci = inc + sin_tab(mph) * idx * INV_SR;
                    x[i] += sin_tab(ph) * lvl;
                    ph += ci;
                    ph -= tp_floorf(ph);
                    mph += minc;
                    if (mph >= 1.0f)
                        mph -= 1.0f;
                    idx *= im;
                    lvl *= pd;
                }
                v->fm_ph = mph;
                v->fm_idx = idx;
                if (v->fm_n > 0 && (v->fm_n -= n) <= 0) {
                    v->fm_n = 0;
                    v->fm_idx = 1.0f;
                }
            } else if (pm == 1.0f && pd == 1.0f)
                for (i = 0; i < n; i++) {
                    x[i] += sin_tab(ph) * lvl;
                    ph += inc;
                    if (ph >= 1.0f)
                        ph -= 1.0f;
                }
            else
                for (i = 0; i < n; i++) {
                    x[i] += sin_tab(ph) * lvl;
                    ph += inc;
                    if (ph >= 1.0f)
                        ph -= 1.0f;
                    inc *= pm;
                    lvl *= pd;
                }
            break;
        case W_TRI:
            if (pm == 1.0f && pd == 1.0f)    /* the common case: a steady note, no partial decaying on its own */
                for (i = 0; i < n; i++) {
                    x[i] += (1.0f - 4.0f * tp_absf(ph - 0.5f)) * lvl;
                    ph += inc;
                    if (ph >= 1.0f)
                        ph -= 1.0f;
                }
            else
                for (i = 0; i < n; i++) {
                    x[i] += (1.0f - 4.0f * tp_absf(ph - 0.5f)) * lvl;
                    ph += inc;
                    if (ph >= 1.0f)
                        ph -= 1.0f;
                    inc *= pm;
                    lvl *= pd;
                }
            break;
        case W_SAW: {
            float rdt = inc > 0.0f ? 1.0f / inc : 0.0f;   /* the block's 1/dt: a glide moves it too little to matter */
            if (pm == 1.0f && pd == 1.0f)
                for (i = 0; i < n; i++) {
                    x[i] += (2.0f * ph - 1.0f - blep_r(ph, inc, rdt)) * lvl;
                    ph += inc;
                    if (ph >= 1.0f)
                        ph -= 1.0f;
                }
            else
                for (i = 0; i < n; i++) {
                    x[i] += (2.0f * ph - 1.0f - blep_r(ph, inc, rdt)) * lvl;
                    ph += inc;
                    if (ph >= 1.0f)
                        ph -= 1.0f;
                    inc *= pm;
                    lvl *= pd;
                }
            break;
        }
        default: {
            float rdt = inc > 0.0f ? 1.0f / inc : 0.0f;
            for (i = 0; i < n; i++) {
                float h = ph + 0.5f;
                if (h >= 1.0f)
                    h -= 1.0f;
                x[i] += ((ph < 0.5f ? 1.0f : -1.0f) + blep_r(ph, inc, rdt) - blep_r(h, inc, rdt)) * lvl;
                ph += inc;
                if (ph >= 1.0f)
                    ph -= 1.0f;
                inc *= pm;
                lvl *= pd;
            }
        }
        }
        v->ph[p] = ph;
        v->inc[p] = inc;
        v->lvl[p] = lvl;
        if (pm != 1.0f && inc0 != 0.0f)      /* a pitch slide under vibrato: the vibrato's centre slides too */
            v->base_inc[p] *= inc / inc0;
    }
    if (v->pr_n > 0 && (v->pr_n -= n) < 0)
        v->pr_n = 0;

    if (v->noise == 1) {
        float g = v->nlvl;
        uint32_t r = v->nr;
        for (i = 0; i < n; i++)
            x[i] += noise_bi(&r) * g;
        v->nr = r;
    } else if (v->noise == 2) {        /* the web app's brown noise: a leaky integrator of white */
        float br = v->brown, g = v->nlvl * 3.5f;
        uint32_t r = v->nr;
        for (i = 0; i < n; i++) {
            br = (br + 0.02f * noise_bi(&r)) * (1.0f / 1.02f);
            x[i] += br * g;
        }
        v->brown = br;
        v->nr = r;
    }

    if (v->fpar && v->ftype[1]) {             /* side by side: both filters hear the voice, and are summed */
        float y[TP_BLOCK];
        for (i = 0; i < n; i++)
            y[i] = x[i];
        svf_run(v, 0, x, n);
        svf_run(v, 1, y, n);
        for (i = 0; i < n; i++)
            x[i] += y[i];
    } else
        for (p = 0; p < 2 && v->ftype[p]; p++)
            svf_run(v, p, x, n);

    bus += i0;
    {   /* the envelope at control rate, interpolated; the tremolo (amp) folded in */
        float e0 = v->env, e1 = env_advance(v, n), a = v->amp;
        if (e1 == e0) {
            float e = e0 * a;
            for (i = 0; i < n; i++)
                bus[i] += x[i] * e;
        } else {
            float e = e0 * a, de = (e1 - e0) * a / (float)n;
            for (i = 0; i < n; i++) {
                e += de;
                bus[i] += x[i] * e;
            }
        }
    }
    if (last || v->stage == ST_DONE)
        v->on = 0;
}

/* ------------------------------------------------------------------- loops --- */
static const tp_bank_t *bank;
static struct {
    uint8_t active, pending, alive, want, cmd, panned, touched, bal;
    float gate, ginc, vol, vgain, pl, pr, rev, dly, pan;
#ifdef TP_METERS
    float level;
#endif
} lp[TP_NBUS];
static float bus[TP_NBUS][TP_BLOCK] TP_BIG;
static float bpm = 72.0f, until_next;
int tp_step;
uint8_t tp_kal[64];

float tp_sps(void) { return 60.0f / bpm / 4.0f; }
/* the chords playing: the bank's own, or a progression of its key (tp_set_chords). A change waits for the next bar
 * line (do_step) unless nothing is playing */
static tp_chord_t chords[4], chords_next[4];
static uint8_t kal_next[64];
static volatile uint8_t chords_pending;

const tp_chord_t *tp_chord_at(int step) { return &chords[(step % 64) / 16]; }
const tp_chord_t *tp_chords(void) { return chords; }
int tp_chords_pending(void) { return chords_pending; }

void tp_set_chords(const tp_chord_t *c, const uint8_t *kal, int now)
{
    if (now) {                               /* nothing sounds: at once */
        memcpy(chords, c, sizeof chords);
        memcpy(tp_kal, kal, sizeof tp_kal);
        chords_pending = 0;
        return;
    }
    if (chords_pending)                      /* the last change has not reached its bar line yet: keep it */
        return;
    memcpy(chords_next, c, sizeof chords_next);
    memcpy(kal_next, kal, sizeof kal_next);
    __asm__ __volatile__("" ::: "memory");
    chords_pending = 1;
}

/* the bank's own note grid: the web app's kalimba grid, from its seed (its own LCG, so the same notes) */
void tp_bank_kal(const tp_bank_t *b, uint8_t *kal)
{
    uint32_t r = b->seed;
    int i;
    for (i = 0; i < 64; i++) {
        float x, y;
        r = r * 1664525u + 1013904223u;
        x = (float)(r >> 8) * (1.0f / 16777216.0f);
        if (x < b->dens) {
            r = r * 1664525u + 1013904223u;
            y = (float)(r >> 8) * (1.0f / 16777216.0f);
            kal[i] = b->penta[(int)(y * (float)b->npenta)];
        } else
            kal[i] = 0;
    }
}

void tp_voice_limit(int n) { vlimit = n < 1 ? 1 : n > TP_MAXVOICE ? TP_MAXVOICE : n; }
void tp_bpm(float b) { bpm = b; }
void tp_toggle(int loop, int on)
{
    lp[loop].want = (uint8_t)on;
    __asm__ __volatile__("" ::: "memory");   /* the render reads cmd, then want: write them in that order */
    lp[loop].cmd = 1;
}
void tp_volume(int loop, float v) { lp[loop].vol = v; }

void tp_param(int loop, int k, float v)
{
    switch (k) {
    case TP_P_VOL: lp[loop].vol = v; break;
    case TP_P_ATT: lt_at[loop] = v; break;
    case TP_P_DEC: lt_de[loop] = v; break;
    case TP_P_REV: lp[loop].rev = v; break;
    case TP_P_ECHO: lp[loop].dly = v; break;
    case TP_P_PAN: lp[loop].pan = v; lp[loop].panned = 1; break;
    }
}

float tp_param_default(int loop, int k)
{
    const tp_loop_t *L = &bank->loops[loop];
    switch (k) {
    case TP_P_VOL: return 0.7f;
    case TP_P_ATT: return 0.0f;
    case TP_P_DEC: return 0.5f;
    case TP_P_REV: return L->rev;
    case TP_P_ECHO: return L->dly;
    default: return L->pan;
    }
}

static void set_pan(int l)
{
    if (lp[l].bal) {                     /* the web app's recorded loops and live keys: centre at unity, panning
                                          * only turns the far side down */
        lp[l].pl = lp[l].pan > 0.0f ? 1.0f - lp[l].pan : 1.0f;
        lp[l].pr = lp[l].pan < 0.0f ? 1.0f + lp[l].pan : 1.0f;
    } else {                             /* the preset loops: StereoPanner with a mono input, equal power */
        float x = (lp[l].pan + 1.0f) * 0.5f;
        lp[l].pl = tp_sin_turns(0.25f - 0.25f * x);
        lp[l].pr = tp_sin_turns(0.25f * x);
    }
    lp[l].panned = 0;
}
static volatile uint8_t realign, fast_stop;
static void load_bank(const tp_bank_t *b);

/* overload: the render took most of its time (the FM-1's app_overload). The n quietest notes fade out in 3 ms;
 * a loop's continuous sounds (drones, rain) are kept. Called from the render's own context. */
int tp_shed(int n)
{
    int done = 0, i;
    while (done < n) {
        tp_voice_t *q = 0;
        float qe = 0.0f;
        for (i = 0; i < TP_MAXVOICE; i++) {
            tp_voice_t *v = &vs[i];
            float e;
            if (!v->on || v->persist || v->shed || v->wait > 0 || v->stage == ST_DONE)
                continue;
            e = v->env * v->amp;
            if (!q || e < qe) {
                q = v;
                qe = e;
            }
        }
        if (!q)
            break;
        q->shed = 1;
        q->stage = ST_REL;
        q->n = q->rel = (int32_t)(0.003f * TP_SR);
        q->rdec = q->env / (float)q->n;
        done++;
    }
    tp_stats.shed += (uint32_t)done;
    return done;
}

/* switching banks, as the web app does: every loop fades out in 0.35 s (tp_stop_fast); once none is sounding
 * (tp_quiet), tp_set_bank loads the new one, and it starts on its first chord. The reverb and echo ring on. */
void tp_stop_fast(void)
{
    int l;
    for (l = 0; l < TP_NLOOP; l++)
        lp[l].want = 0;
    __asm__ __volatile__("" ::: "memory");
    fast_stop = 1;
}

int tp_quiet(void)
{
    int l;
    if (fast_stop)
        return 0;
    for (l = 0; l < TP_NLOOP; l++)
        if (lp[l].alive || lp[l].active || lp[l].pending)
            return 0;
    return 1;
}

void tp_set_bank(const tp_bank_t *b)
{
    load_bank(b);
    __asm__ __volatile__("" ::: "memory");
    realign = 1;
}

const tp_bank_t *tp_bank_now(void) { return bank; }

int tp_loop_state(int loop) { return lp[loop].active | lp[loop].pending << 1 | lp[loop].alive << 2; }
#ifdef TP_METERS
float tp_loop_level(int loop) { return lp[loop].alive ? lp[loop].level : 0.0f; }
#endif

static void kill_loop_voices(int loop)
{
    int i;
    for (i = 0; i < TP_MAXVOICE; i++)
        if (vs[i].on && vs[i].loop == loop)
            vs[i].on = 0;
}

static void activate(int l, int off)
{
    if (lp[l].alive && lp[l].gate > 0.0f)
        kill_loop_voices(l);           /* restarted while fading out (the web app keeps both briefly) */
    lp[l].active = 1;
    lp[l].pending = 0;
    lp[l].alive = 1;
    lp[l].gate = 0.0f;
    lp[l].ginc = 1.0f / (bank->loops[l].fade * TP_SR);
    if (bank->loops[l].start)
        bank->loops[l].start(l, off);
}

/* --------------------------------------------- the keys and the recorder --- */
/* events from the main loop, in order: notes on and off, and the recorder's commands */
enum { LQ_ON = 1, LQ_OFF, LQ_ALLOFF, LQ_ARM, LQ_FIN, LQ_CANCEL };
#define LQ_N 64
static struct { uint8_t type, a, midi, b; uint16_t id; } lq[LQ_N];
static volatile uint8_t lq_w, lq_r;
static uint16_t lq_id;
static uint8_t id_note[256];                     /* a held note's pitch, by its id (for the recorder) */

static int lq_put(uint8_t type, uint8_t a, uint8_t midi, uint8_t b, uint16_t id)
{
    uint8_t w = lq_w, nw = (uint8_t)((w + 1u) % LQ_N);
    if (nw == lq_r)
        return 0;                                /* full: the event is lost (a key's note off is resent below) */
    lq[w].type = type;
    lq[w].a = a;
    lq[w].midi = midi;
    lq[w].b = b;
    lq[w].id = id;
    __asm__ __volatile__("" ::: "memory");
    lq_w = nw;
    return 1;
}

uint16_t tp_live_on(int instr, int midi, int vel)
{
    uint16_t id = ++lq_id ? lq_id : ++lq_id;
    return lq_put(LQ_ON, (uint8_t)instr, (uint8_t)midi, (uint8_t)vel, id) ? id : 0;
}
void tp_live_off(uint16_t id)
{
    if (id)
        lq_put(LQ_OFF, 0, 0, 0, id);
}
void tp_live_all_off(void) { lq_put(LQ_ALLOFF, 0, 0, 0, 0); }
void tp_rec_arm(int slot, int click) { lq_put(LQ_ARM, (uint8_t)slot, 0, (uint8_t)click, 0); }
void tp_rec_finish(int keep_playing) { lq_put(LQ_FIN, 0, 0, (uint8_t)keep_playing, 0); }
void tp_rec_cancel(void) { lq_put(LQ_CANCEL, 0, 0, 0, 0); }

static int rec_state, rec_slot, rec_len, rec_click;
static int32_t rec_R;                            /* the step the take starts on */
static struct { int32_t on; uint8_t vel, open; } ropen[128];

void tp_rec_info(tp_rec_info_t *r)
{
    int32_t s = tp_step;
    r->state = rec_state;
    r->slot = rec_slot;
    r->pass = rec_state == TP_REC_ON && rec_len ? (int)((s - rec_R) / rec_len) : 0;
    r->beats_to_go = rec_state == TP_REC_COUNTIN ? (int)((rec_R - s + 3) / 4) : 0;
    r->pos = rec_state == TP_REC_ON && rec_len ? (float)(s % rec_len) : 0.0f;   /* as the notes go in (rec_close)
                                                * and play: on the shared cycle, not from where the take began */
}

static void rec_close(int midi, int32_t off256)  /* a note let go: into the take, at its place in the loop */
{
    tp_user_t *U = &tp_user[rec_slot];
    int32_t len256 = rec_len * 256, on = ropen[midi].on, d = off256 - on;
    if (!ropen[midi].open)
        return;
    ropen[midi].open = 0;
    if (d < 38)                                  /* at least 0.15 of a step, as the web app */
        d = 38;
    if (d > len256)
        d = len256;
    if (U->n < TP_MAXNOTES) {
        tp_note_t *N = &U->notes[U->n];
        N->pos = (uint16_t)(((on % len256) + len256) % len256);   /* the loop's place: on the shared cycle */
        N->dur = (uint16_t)d;
        N->note = (uint8_t)midi;
        N->vel = ropen[midi].vel;
        __asm__ __volatile__("" ::: "memory");
        U->n++;
    }
}

static void rec_stop_slot(float fade)
{
    int l = rec_slot;
    lp[l].want = 0;
    lp[l].pending = 0;
    if (lp[l].active) {
        lp[l].active = 0;
        lp[l].ginc = -1.0f / (fade * TP_SR);
    }
}

static void click(int off, int accent, int soft)   /* the count-in and the click: the web app's */
{
    tp_voice_t *v = tv_new(TP_CLICK, off);
    tv_osc(v, W_SINE, accent ? 1760.0f : 1175.0f, 0.0f, 1.0f);
    tv_env_ad(v, 0.001f, (accent ? 0.35f : 0.2f) * (soft ? 0.45f : 1.0f), 0.05f);
    tv_len(v, 0.1f);
}

/* the events, at the start of a block: now is where the block starts, in 1/256 of a step */
static void live_events(float sps_n)
{
    int32_t now = tp_step * 256 - (int32_t)(until_next / sps_n * 256.0f);
    int i;
    while (lq_r != lq_w) {
        uint8_t k = lq_r;
        switch (lq[k].type) {
        case LQ_ON: {
            int m = lq[k].midi & 127;
            tv_set_id(tp_instr_note(TP_LIVE, 0, lq[k].a, m, (float)lq[k].b * (1.0f / 127.0f), -1.0f), lq[k].id);
            id_note[lq[k].id & 255u] = (uint8_t)m;
            if (rec_state == TP_REC_ON || (rec_state == TP_REC_COUNTIN && now >= rec_R * 256 - 128)) {
                if (ropen[m].open)               /* (pressed again before its release reached us) */
                    rec_close(m, now);
                ropen[m].on = now > rec_R * 256 ? now : rec_R * 256;   /* a note a touch early lands on the one */
                ropen[m].vel = lq[k].b;
                ropen[m].open = 1;
            }
            break;
        }
        case LQ_OFF:
        case LQ_ALLOFF:
            for (i = 0; i < TP_MAXVOICE; i++) {
                tp_voice_t *v = &vs[i];
                if (v->on && v->loop == TP_LIVE && v->etype == E_GATE && v->stage != ST_REL && v->stage != ST_DONE &&
                    (lq[k].type == LQ_ALLOFF || v->nid == lq[k].id)) {
                    v->stage = ST_REL;           /* let go: out over its release, from where it is */
                    v->n = v->rel;
                    v->rdec = v->env / (float)v->rel;
                }
            }
            if (rec_state == TP_REC_ON) {
                if (lq[k].type == LQ_OFF)
                    rec_close(id_note[lq[k].id & 255u], now);
                else
                    for (i = 0; i < 128; i++)
                        rec_close(i, now);
            }
            break;
        case LQ_ARM:
            if (lq[k].a < TP_NLOOP && rec_state == TP_REC_OFF) {
                tp_user_t *U = &tp_user[lq[k].a];
                rec_slot = lq[k].a;
                rec_click = lq[k].b;
                rec_len = (U->bars ? U->bars : 2) * 16;
                rec_R = (tp_step + 15) / 16 * 16 + 16;   /* a bar's count-in from the next bar line */
                U->n = 0;
                U->used = 1;
                for (i = 0; i < 128; i++)
                    ropen[i].open = 0;
                rec_state = TP_REC_COUNTIN;
            }
            break;
        case LQ_FIN:
            if (rec_state == TP_REC_ON) {
                for (i = 0; i < 128; i++)
                    rec_close(i, now);
                rec_state = TP_REC_OFF;
                if (!tp_user[rec_slot].n)
                    tp_user[rec_slot].used = 0;
                if (!lq[k].b || !tp_user[rec_slot].used)
                    rec_stop_slot(0.3f);
                break;
            }                                    /* (finished in the count-in: nothing was taken) */
            /* fall through */
        case LQ_CANCEL:
            if (rec_state != TP_REC_OFF) {
                rec_state = TP_REC_OFF;
                tp_user[rec_slot].used = 0;
                tp_user[rec_slot].n = 0;
                for (i = 0; i < 128; i++)
                    ropen[i].open = 0;
                rec_stop_slot(0.1f);
            }
            break;
        }
        lq_r = (uint8_t)((k + 1u) % LQ_N);
    }
}

static void do_step(int off)
{
    int s = tp_step, l;
    if (rec_state == TP_REC_COUNTIN) {        /* the count-in: a bar of clicks, then the take starts */
        if (s >= rec_R - 16 && s < rec_R && s % 4 == 0)
            click(off, s % 16 == 0, 0);
        if (s == rec_R) {
            rec_state = TP_REC_ON;
            lp[rec_slot].want = 1;
            lp[rec_slot].pending = 0;
            if (!lp[rec_slot].active)
                activate(rec_slot, off);
        }
    } else if (rec_state == TP_REC_ON && rec_click && s % 4 == 0)
        click(off, s % 16 == 0, 1);
    if (chords_pending && s % 16 == 0) {   /* a new progression: from this bar line */
        memcpy(chords, chords_next, sizeof chords);
        memcpy(tp_kal, kal_next, sizeof tp_kal);
        chords_pending = 0;
    }
    for (l = 0; l < TP_NLOOP; l++) {
        if (lp[l].pending && s % (bank->canvas ? 16 : 4) == 0)   /* a canvas's loops join on the bar */
            activate(l, off);
        if (lp[l].active && bank->loops[l].sched)
            bank->loops[l].sched(l, s, off);
    }
    tp_step++;
}

/* ---------------------------------------------------------------- effects --- */
/* Reverb. The web app convolves with 4.5 s of decaying stereo noise: no device does that in real
 * time. This is an 8-line feedback delay network (int16 lines, 43 KB) tuned to the same decay and
 * level: a dense, bright tail, two input diffusers per side. */
#define RV_N 8
static const uint16_t RV_LEN[RV_N] = { 2053, 2251, 2399, 2617, 2791, 3001, 3203, 3407 };
#define RV_TOTAL (2053 + 2251 + 2399 + 2617 + 2791 + 3001 + 3203 + 3407)
static float rv_buf[RV_TOTAL] TP_BIG;      /* float lines: no conversion or clamp on each read and write (87 KB) */
static uint16_t rv_off[RV_N], rv_pos[RV_N];
static float rv_g[RV_N], rv_lpst[RV_N];
#define AP_N 4
static const uint16_t AP_LEN[AP_N] = { 142, 379, 151, 397 };
static float ap_buf[142 + 379 + 151 + 397] TP_BIG;
static uint16_t ap_off[AP_N], ap_pos[AP_N];
float tp_rv_gain = 0.180f;            /* output level: matched to the web convolver's energy (tp_bench rvcal) */
#define RV_RT60 3.6f
#define RV_DAMP 0.82f
#define LINE_SCALE 8192.0f            /* int16 lines hold +-4.0 */

static inline int16_t sat16(float x)
{
    int32_t i = (int32_t)(x * LINE_SCALE);
    return (int16_t)(i > 32767 ? 32767 : i < -32768 ? -32768 : i);
}

static inline float allpass(int j, float x)
{
    float *b = ap_buf + ap_off[j], d = b[ap_pos[j]], y = d - 0.6f * x;
    b[ap_pos[j]] = x + 0.6f * y;
    if (++ap_pos[j] >= AP_LEN[j])
        ap_pos[j] = 0;
    return y;
}

static void reverb(float inl, float inr, float *outl, float *outr)
{
    float y[RV_N], sum = 0.0f, l = 0.0f, r = 0.0f;
    int i;
    inl = allpass(1, allpass(0, inl + 1e-18f));   /* the DC whisper keeps the tails above the denormals, which */
    inr = allpass(3, allpass(2, inr + 1e-18f));   /* many FPUs handle slowly; it is 360 dB down */
    for (i = 0; i < RV_N; i++) {
        float d = rv_buf[rv_off[i] + rv_pos[i]];
        rv_lpst[i] += RV_DAMP * (d - rv_lpst[i]);
        y[i] = rv_lpst[i];
        sum += y[i];
    }
    sum *= 2.0f / RV_N;               /* Householder: y - 2/N sum */
    for (i = 0; i < RV_N; i++) {
        float in = (i & 1) ? inr : inl;
        rv_buf[rv_off[i] + rv_pos[i]] = rv_g[i] * (y[i] - sum) + in;
        if (++rv_pos[i] >= RV_LEN[i])
            rv_pos[i] = 0;
        if (i & 1)
            r += y[i];
        else
            l += y[i];
    }
    *outl = l * tp_rv_gain;
    *outr = r * tp_rv_gain;
}

/* Delay: three steps, fed back at 0.38 through a 2.6 kHz low-pass, as the web app (int16, 141 KB). */
#define DL_MAX 36000                  /* 3 steps at 56 bpm, the slowest tempo, is 35,438 samples */
static int16_t dl_buf[2][DL_MAX] TP_BIG;
static int dl_pos;
static float dl_s1[2], dl_s2[2], dl_a1, dl_a2, dl_a3, dl_k;

static inline float dl_lp(int c, float x)
{
    float v3 = x + 1e-18f - dl_s2[c], v1 = dl_a1 * dl_s1[c] + dl_a2 * v3, v2 = dl_s2[c] + dl_a2 * dl_s1[c] + dl_a3 * v3;
    dl_s1[c] = 2.0f * v1 - dl_s1[c];
    dl_s2[c] = 2.0f * v2 - dl_s2[c];
    return v2;
}

/* Compressor: the web app's DynamicsCompressor settings (-18 dB, 3:1, knee 30, 20 ms / 300 ms),
 * at control rate on the block's peak, with Chromium's knee shape and a measured makeup gain. */
static float cp_gr, cp_gain = 1.0f;
#define CP_T -18.0f
#define CP_R 3.0f
#define CP_W 30.0f
#define CP_MAKEUP 1.10f             /* matched to the web app: solo loops and full mixes of all five banks (README) */

static float comp_curve(float x)       /* Chromium's shape: the knee starts at the threshold and runs W above it */
{
    float o = x - CP_T;
    if (o <= 0.0f)
        return x;
    if (o < CP_W)
        return x + (1.0f / CP_R - 1.0f) * o * o / (2.0f * CP_W);
    return CP_T + CP_W + (1.0f / CP_R - 1.0f) * CP_W * 0.5f + (o - CP_W) / CP_R;
}

/* ------------------------------------------------------------------ render --- */
static float master = 0.8f;
void tp_master(float v) { master = v; }

/* a bank's own data: its loops' settings as designed, and its note grid. Nothing of the old bank may be sounding. */
static void load_bank(const tp_bank_t *b)
{
    int i;
    bank = b;
    tp_bank_kal(b, tp_kal);
    memcpy(chords, b->chords, sizeof chords);
    chords_pending = 0;
    for (i = 0; i < TP_NLOOP; i++) {
        lp[i].vol = 0.7f;
        lp[i].vgain = 0.49f;
        lp[i].rev = b->loops[i].rev;
        lp[i].dly = b->loops[i].dly;
        lp[i].pan = b->loops[i].pan;
        lt_at[i] = 0.0f;
        lt_de[i] = 0.5f;
        lp[i].bal = b->canvas;               /* a canvas's loops pan and send as the web app's recorded ones */
        set_pan(i);
    }
}


void tp_init(const tp_bank_t *b, uint32_t seed)
{
    int i, off = 0;
    float g;
    memset(vs, 0, sizeof vs);
    memset(lp, 0, sizeof lp);
    for (i = 0; i <= SIN_N; i++)
        sintab[i] = tp_sin_turns((float)i / (float)SIN_N);
    memset(&tp_stats, 0, sizeof tp_stats);
    rng = seed ? seed : 1u;
    rec_state = TP_REC_OFF;
    lq_r = lq_w;
    for (i = TP_NLOOP; i < TP_NBUS; i++) {        /* the keys (the web app's liveBus) and the click: always open */
        lp[i].alive = lp[i].active = 1;
        lp[i].gate = 1.0f;
        lp[i].bal = 1;
        lp[i].vol = i == TP_LIVE ? 0.948683f : 1.0f;   /* liveBus: 0.9 */
        lp[i].vgain = lp[i].vol * lp[i].vol;
        lp[i].rev = i == TP_LIVE ? 0.35f : 0.0f;
        lp[i].dly = i == TP_LIVE ? 0.2f : 0.0f;
        lt_at[i] = 0.0f;
        lt_de[i] = 0.5f;
        set_pan(i);
    }
    tp_step = 0;
    until_next = 0.08f * TP_SR;
    realign = fast_stop = 0;
    load_bank(b);
    memset(rv_buf, 0, sizeof rv_buf);
    memset(rv_lpst, 0, sizeof rv_lpst);
    for (i = 0; i < RV_N; i++) {
        rv_off[i] = (uint16_t)off;
        off += RV_LEN[i];
        rv_pos[i] = 0;
        rv_g[i] = tp_exp2f(-3.0f * 3.32192809f * (float)RV_LEN[i] / (RV_RT60 * TP_SR));
    }
    memset(ap_buf, 0, sizeof ap_buf);
    for (i = 0, off = 0; i < AP_N; i++) {
        ap_off[i] = (uint16_t)off;
        off += AP_LEN[i];
        ap_pos[i] = 0;
    }
    memset(dl_buf, 0, sizeof dl_buf);
    memset(dl_s1, 0, sizeof dl_s1);
    memset(dl_s2, 0, sizeof dl_s2);
    dl_pos = 0;
    dl_k = 1.0f / tp_db2lin(0.7f);
    g = tp_tan_pi(2600.0f * INV_SR);
    dl_a1 = 1.0f / (1.0f + g * (g + dl_k));
    dl_a2 = g * dl_a1;
    dl_a3 = g * dl_a2;
    cp_gr = 0.0f;
    cp_gain = CP_MAKEUP;
}

#ifdef TP_METERS
#define METER(x) x
#else
#define METER(x)
#endif

#ifdef TP_PROFILE
uint32_t (*tp_prof_clock)(void);
uint32_t tp_prof[TP_PROF_N];
#define PROF(k, t) do { uint32_t t_ = tp_prof_clock(); tp_prof[k] += t_ - (t); (t) = t_; } while (0)
#else
#define PROF(k, t) ((void)0)
#endif

void tp_render(float *out, int n)
{
    int b, i, l;
    for (b = 0; b < n; b += TP_BLOCK) {
        float dl[TP_BLOCK], dr[TP_BLOCK], rl[TP_BLOCK], rr[TP_BLOCK], el[TP_BLOCK], er[TP_BLOCK];
        float sps_n = tp_sps() * TP_SR, peak = 0.0f, g0, g1, gstep;
        int dlen = (int)(3.0f * sps_n + 0.5f), sounding = 0;
        if (dlen > DL_MAX - 1)
            dlen = DL_MAX - 1;

        if (fast_stop) {                   /* a bank switch: everything out in 0.35 s */
            for (l = 0; l < TP_NLOOP; l++) {
                lp[l].cmd = 0;
                lp[l].pending = 0;
                if (lp[l].active || lp[l].alive) {
                    lp[l].active = 0;
                    lp[l].ginc = -1.0f / (0.35f * TP_SR);
                }
            }
            fast_stop = 0;
        }
        if (realign) {                     /* a new bank begins on its first chord */
            tp_step = (tp_step + 63) / 64 * 64;
            realign = 0;
        }
        for (l = 0; l < TP_NLOOP; l++) {   /* commands from the main loop */
            if (!lp[l].cmd)
                continue;
            lp[l].cmd = 0;
            if (lp[l].want) {
                if (!lp[l].active)
                    lp[l].pending = 1;
            } else {
                lp[l].pending = 0;
                if (lp[l].active) {
                    lp[l].active = 0;
                    lp[l].ginc = -1.0f / (1.2f * TP_SR);
                }
            }
        }
        if (lq_r != lq_w)
            live_events(sps_n);
        while (until_next < (float)TP_BLOCK) {   /* the steps that fall in this block */
            do_step((int)until_next);
            until_next += sps_n;
        }
        until_next -= (float)TP_BLOCK;

#ifdef TP_PROFILE
        uint32_t pt = tp_prof_clock();       /* the stages' time, for the device's own measurement */
#endif
        for (l = 0; l < TP_NBUS; l++)
            lp[l].touched = 0;
        for (i = 0; i < TP_MAXVOICE; i++)     /* the loops that sound in this block: only their buses are mixed */
            if (vs[i].on && vs[i].wait < TP_BLOCK)
                lp[vs[i].loop].touched = 1;
        for (l = 0; l < TP_NBUS; l++)
            if (lp[l].touched)
                memset(bus[l], 0, sizeof bus[l]);
        for (i = 0; i < TP_MAXVOICE; i++)
            if (vs[i].on) {
                sounding++;
                voice_block(&vs[i], bus[vs[i].loop]);
            }
        PROF(TP_PROF_VOICES, pt);

        memset(dl, 0, sizeof dl); memset(dr, 0, sizeof dr);
        memset(rl, 0, sizeof rl); memset(rr, 0, sizeof rr);
        memset(el, 0, sizeof el); memset(er, 0, sizeof er);
        for (l = 0; l < TP_NBUS; l++) {
            float g0, g1, gs, a, b, ra, rb, ea, eb, *x = bus[l];
            if (!lp[l].alive)
                continue;
            if (lp[l].panned)
                set_pan(l);
            lp[l].vgain += (lp[l].vol * lp[l].vol - lp[l].vgain) * 0.5f;
            g0 = lp[l].gate;                 /* the gate's ramp over the block, clamped at its ends */
            g1 = g0 + lp[l].ginc * (float)TP_BLOCK;
            g1 = g1 > 1.0f ? 1.0f : g1 < 0.0f ? 0.0f : g1;
            lp[l].gate = g1;
            if (lp[l].touched) {             /* a silent loop adds nothing: its gate still moves */
                METER(float sum2 = 0.0f;)
                gs = (g1 - g0) * (1.0f / TP_BLOCK);
                a = lp[l].pl * lp[l].vgain;
                b = lp[l].pr * lp[l].vgain;
                ra = a * lp[l].rev;
                rb = b * lp[l].rev;
                if (lp[l].bal)               /* the web app sends a recorded loop to the effects before its panner */
                    ra = rb = lp[l].vgain * lp[l].rev;
                if (lp[l].dly > 0.0f) {
                    ea = a * lp[l].dly;
                    eb = b * lp[l].dly;
                    if (lp[l].bal)
                        ea = eb = lp[l].vgain * lp[l].dly;
                    for (i = 0; i < TP_BLOCK; i++) {
                        float s = x[i] * (g0 += gs);
                        METER(sum2 += s * s;)
                        dl[i] += s * a;
                        dr[i] += s * b;
                        rl[i] += s * ra;
                        rr[i] += s * rb;
                        el[i] += s * ea;
                        er[i] += s * eb;
                    }
                } else
                    for (i = 0; i < TP_BLOCK; i++) {
                        float s = x[i] * (g0 += gs);
                        METER(sum2 += s * s;)
                        dl[i] += s * a;
                        dr[i] += s * b;
                        rl[i] += s * ra;
                        rr[i] += s * rb;
                    }
#ifdef TP_METERS
                lp[l].level += (sum2 * (1.0f / TP_BLOCK) - lp[l].level) * 0.016f;   /* 0.0005 a sample */
            } else {
                lp[l].level *= 0.984f;
#endif
            }
            if (!lp[l].active && g1 <= 0.0f) {   /* faded out: free its voices */
                kill_loop_voices(l);
                lp[l].alive = 0;
            }
        }
        PROF(TP_PROF_MIX, pt);

        for (i = 0; i < TP_BLOCK; i++) {
            float yl, yr, ol, or_, wl, wr;
            int rd = dl_pos - dlen;
            if (rd < 0)
                rd += DL_MAX;
            yl = dl_lp(0, (float)dl_buf[0][rd] * (1.0f / LINE_SCALE));
            yr = dl_lp(1, (float)dl_buf[1][rd] * (1.0f / LINE_SCALE));
            dl_buf[0][dl_pos] = sat16(el[i] + 0.38f * yl);
            dl_buf[1][dl_pos] = sat16(er[i] + 0.38f * yr);
            if (++dl_pos >= DL_MAX)
                dl_pos = 0;
            yl *= 0.5f;
            yr *= 0.5f;
            reverb(rl[i] + yl, rr[i] + yr, &wl, &wr);
            ol = dl[i] + yl + 0.9f * wl;
            or_ = dr[i] + yr + 0.9f * wr;
            out[2 * (b + i)] = ol;
            out[2 * (b + i) + 1] = or_;
            if (tp_absf(ol) > peak)
                peak = tp_absf(ol);
            if (tp_absf(or_) > peak)
                peak = tp_absf(or_);
        }

        PROF(TP_PROF_FX, pt);
        {   /* the compressor, at control rate */
            float x = tp_lin2db(peak + 1e-9f), gr = comp_curve(x) - x;
            float c = gr < cp_gr ? 0.0703f : 0.0048f;   /* 1 - e^(-32 / (0.02 sr)), ... (0.3 sr) */
            cp_gr += (gr - cp_gr) * c;
            g0 = cp_gain;
            g1 = tp_db2lin(cp_gr) * CP_MAKEUP;
            cp_gain = g1;
            gstep = (g1 - g0) * (1.0f / TP_BLOCK);
        }
        for (i = 0; i < TP_BLOCK; i++) {
            float g = (g0 + gstep * (float)(i + 1)) * master;
            out[2 * (b + i)] *= g;
            out[2 * (b + i) + 1] *= g;
        }

        PROF(TP_PROF_COMP, pt);
        tp_stats.sounding = sounding;
        if (sounding > tp_stats.peak)
            tp_stats.peak = sounding;
        tp_stats.blocks++;
        tp_stats.voice_blocks += (uint32_t)sounding;
    }
}

/* for the host bench: the reverb on its own */
void tp_reverb_probe(float inl, float inr, float *outl, float *outr) { reverb(inl, inr, outl, outr); }
