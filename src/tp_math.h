/* Tidepool on the FM-1: single-precision maths with no libm.
 * The FM-1's FPU is float-only and its firmwares link no maths library, so this is all the engine
 * uses. Build the host with -ffp-contract=off (the device has no fused multiply-add). */
#pragma once
#include <stdint.h>

typedef union { float f; uint32_t u; int32_t i; } tp_bits_t;

static inline float tp_floorf(float x)
{
    int32_t i = (int32_t)x;
    float f = (float)i;
    return f > x ? f - 1.0f : f;
}

static inline float tp_absf(float x) { tp_bits_t b; b.f = x; b.u &= 0x7FFFFFFFu; return b.f; }

/* 2^x: x = n + f with |f| <= 0.5, Taylor to f^6 (rel ~1e-7) */
static inline float tp_exp2f(float x)
{
    tp_bits_t b;
    float n, f, p;
    if (x < -126.0f)
        return 0.0f;
    if (x > 127.0f)
        x = 127.0f;
    n = tp_floorf(x + 0.5f);
    f = x - n;
    p = 1.540353e-4f;
    p = p * f + 1.333356e-3f;
    p = p * f + 9.618129e-3f;
    p = p * f + 5.550411e-2f;
    p = p * f + 2.402265e-1f;
    p = p * f + 6.931472e-1f;
    p = p * f + 1.0f;
    b.f = p;
    b.i += (int32_t)n << 23;
    return b.f;
}

/* log2(x), x > 0: exponent plus 2/ln2 * atanh((m-1)/(m+1)), m centred on 1 */
static inline float tp_log2f(float x)
{
    tp_bits_t b;
    float m, t, t2, p;
    int32_t e;
    b.f = x;
    if (b.i <= 0)
        return -127.0f;
    e = ((b.i >> 23) & 0xFF) - 127;
    b.i = (b.i & 0x007FFFFF) | 0x3F800000;
    m = b.f;
    if (m > 1.41421356f) {
        m *= 0.5f;
        e += 1;
    }
    t = (m - 1.0f) / (m + 1.0f);
    t2 = t * t;
    p = 2.0f / 9.0f;
    p = p * t2 + 2.0f / 7.0f;
    p = p * t2 + 2.0f / 5.0f;
    p = p * t2 + 2.0f / 3.0f;
    p = p * t2 + 2.0f;
    return (float)e + p * t * 1.44269504f;
}

static inline float tp_powf(float x, float y) { return x <= 0.0f ? 0.0f : tp_exp2f(y * tp_log2f(x)); }

/* sin(2 pi x) for a phase in turns: fold to a quarter wave, Taylor to x^11 (abs ~6e-8) */
static inline float tp_sin_turns(float ph)
{
    float x = ph - tp_floorf(ph + 0.5f), x2, p;    /* [-0.5, 0.5) */
    if (x > 0.25f)
        x = 0.5f - x;
    else if (x < -0.25f)
        x = -0.5f - x;
    x *= 6.28318531f;                              /* [-pi/2, pi/2] */
    x2 = x * x;
    p = -2.5052108e-8f;
    p = p * x2 + 2.7557319e-6f;
    p = p * x2 - 1.9841270e-4f;
    p = p * x2 + 8.3333333e-3f;
    p = p * x2 - 1.6666667e-1f;
    p = p * x2 + 1.0f;
    return p * x;
}

/* tan(pi * r) for r in [0, 0.5): the filter warp, sin over cos */
static inline float tp_tan_pi(float r) { return tp_sin_turns(0.5f * r) / tp_sin_turns(0.25f - 0.5f * r); }

static inline float tp_mtof(float m) { return 440.0f * tp_exp2f((m - 69.0f) * (1.0f / 12.0f)); }
static inline float tp_cents(float c) { return tp_exp2f(c * (1.0f / 1200.0f)); }
static inline float tp_db2lin(float db) { return tp_exp2f(db * 0.166096405f); }
static inline float tp_lin2db(float x) { return tp_log2f(x) * 6.02059991f; }
