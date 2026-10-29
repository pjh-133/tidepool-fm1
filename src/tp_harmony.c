/* Tidepool on the FM-1: new chord progressions for a bank (see tp_harmony.h). Integer only. */
#include "tp_harmony.h"

static const uint8_t SCALE[2][7] = { { 0, 2, 4, 5, 7, 9, 11 },   /* major (ionian) */
                                     { 0, 2, 3, 5, 7, 8, 10 } }; /* minor (aeolian) */

/* how likely each degree follows another: functional harmony's pulls, and what these banks already do
 * (Shallows i VI III VII, Neon Rain i VI III VII, Dungeon Synth i VI VII v, Forest Floor I vi IV V). The diminished
 * chord (vii in major, ii in minor) never comes */
static const uint8_t PULL[2][7][7] = {
    {   /* major:  I  ii iii IV  V  vi vii */
        { 0, 2, 1, 4, 3, 4, 0 },   /* from I */
        { 1, 0, 1, 2, 4, 1, 0 },   /* ii */
        { 1, 1, 0, 3, 1, 4, 0 },   /* iii */
        { 3, 2, 1, 0, 4, 2, 0 },   /* IV */
        { 4, 1, 1, 2, 0, 4, 0 },   /* V */
        { 2, 3, 1, 4, 3, 0, 0 },   /* vi */
        { 1, 0, 0, 0, 0, 0, 0 },   /* vii (never reached) */
    },
    {   /* minor:  i  ii III iv  v  VI VII */
        { 0, 0, 2, 3, 2, 4, 3 },   /* from i */
        { 1, 0, 0, 0, 0, 0, 0 },   /* ii (never reached) */
        { 2, 0, 0, 3, 1, 4, 3 },   /* III */
        { 3, 0, 2, 0, 3, 2, 3 },   /* iv */
        { 4, 0, 1, 2, 0, 3, 1 },   /* v */
        { 2, 0, 3, 3, 2, 0, 4 },   /* VI */
        { 3, 0, 4, 2, 1, 3, 0 },   /* VII */
    },
};

static uint32_t hr;
static uint32_t hrand(uint32_t n)
{
    hr = hr * 1664525u + 1013904223u;
    return (hr >> 8) % n;
}

static int pick(const uint8_t *w, int n)        /* an index, by weight; -1 if every weight is 0 */
{
    uint32_t tot = 0, r;
    int i;
    for (i = 0; i < n; i++)
        tot += w[i];
    if (!tot)
        return -1;
    r = hrand(tot);
    for (i = 0; i < n; i++) {
        if (r < w[i])
            return i;
        r -= w[i];
    }
    return n - 1;
}

static const char *const SHARP[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
static const char *const FLAT[12] = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };

static void cat(char *d, const char *s, int max)
{
    int n = 0;
    while (d[n])
        n++;
    while (*s && n < max - 1)
        d[n++] = *s++;
    d[n] = 0;
}

/* chord types: which scale degrees above the root (0 the root, 2 the third, 4 the fifth, 6 the seventh, 1 the second
 * or ninth, 3 the fourth, 5 the sixth) */
enum { CT_TRIAD, CT_SEVENTH, CT_NINTH, CT_SIXNINE, CT_SUS2, CT_SUS4, CT_ADD9, CT_N };
static const int8_t CT_DEG[CT_N][5] = {
    { 0, 2, 4, -1, -1 }, { 0, 2, 4, 6, -1 }, { 0, 2, 4, 6, 1 }, { 0, 2, 4, 5, 1 },
    { 0, 1, 4, -1, -1 }, { 0, 3, 4, -1, -1 }, { 0, 2, 4, 1, -1 },
};
/* each style's taste for them */
static const uint8_t STYLE_W[3][CT_N] = {
    { 0, 3, 5, 2, 1, 0, 0 },   /* lush: 7ths, 9ths, 6/9, a little sus2 */
    { 2, 2, 0, 0, 0, 2, 3 },   /* folk: triads, 7ths, sus4, add9 */
    { 6, 0, 0, 0, 0, 1, 0 },   /* triads, now and then a sus4 */
};

/* the pitch class of scale degree d above degree root, in key (tonic, minor) */
static int deg_pc(int tonic, int minor, int root, int d) { return (tonic + SCALE[minor][(root + d) % 7]) % 12; }

static int interval(int a, int b) { return ((b - a) % 12 + 12) % 12; }

/* a chord type that sounds right on this degree: a 9th or a sus2 only over a whole step (no b9), a sus4 only over a
 * perfect fourth, a 6/9 only on a major chord */
static int type_ok(int tonic, int minor, int root, int t)
{
    int r = deg_pc(tonic, minor, root, 0), third = interval(r, deg_pc(tonic, minor, root, 2));
    int second = interval(r, deg_pc(tonic, minor, root, 1)), fourth = interval(r, deg_pc(tonic, minor, root, 3));
    if ((t == CT_NINTH || t == CT_SUS2 || t == CT_SIXNINE || t == CT_ADD9) && second != 2)
        return 0;
    if (t == CT_SUS4 && fourth != 5)
        return 0;
    if (t == CT_SIXNINE && third != 4)
        return 0;
    return 1;
}

static void chord_name(int tonic, int minor, int root, int t, char *out)
{
    int r = deg_pc(tonic, minor, root, 0), third = interval(r, deg_pc(tonic, minor, root, 2));
    int seventh = interval(r, deg_pc(tonic, minor, root, 6));
    /* flats in the flat keys (D minor and its relatives), sharps elsewhere */
    int flat = minor ? (tonic == 2 || tonic == 7 || tonic == 0 || tonic == 5 || tonic == 10 || tonic == 3)
                     : (tonic == 5 || tonic == 10 || tonic == 3 || tonic == 8 || tonic == 1);
    out[0] = 0;
    cat(out, (flat ? FLAT : SHARP)[r], 8);
    switch (t) {
    case CT_TRIAD: cat(out, third == 3 ? "m" : "", 8); break;
    case CT_SEVENTH: cat(out, third == 3 ? "m7" : seventh == 11 ? "maj7" : "7", 8); break;
    case CT_NINTH: cat(out, third == 3 ? "m9" : seventh == 11 ? "maj9" : "9", 8); break;
    case CT_SIXNINE: cat(out, "6/9", 8); break;
    case CT_SUS2: cat(out, "sus2", 8); break;
    case CT_SUS4: cat(out, "sus4", 8); break;
    default: cat(out, third == 3 ? "madd9" : "add9", 8); break;
    }
}

/* five notes, a close stack of the chord's tones, from the lowest that moves least from prev */
static void voice(const int *pcs, int npc, const uint8_t *prev, uint8_t *out)
{
    int best_cost = 1 << 30, b, k;
    uint8_t cand[5];
    for (b = 45; b <= 57; b++) {
        int ok = 0, cost = 0, n = b;
        for (k = 0; k < npc; k++)
            if (b % 12 == pcs[k])
                ok = 1;
        if (!ok)
            continue;
        cand[0] = (uint8_t)b;
        for (k = 1; k < 5; k++) {                /* the next chord tone up */
            int m, j, hit = 0;
            for (m = n + 1; !hit; m++)
                for (j = 0; j < npc; j++)
                    if (m % 12 == pcs[j]) {
                        hit = 1;
                        n = m;
                        break;
                    }
            cand[k] = (uint8_t)n;
        }
        for (k = 0; k < 5; k++)
            cost += cand[k] > prev[k] ? cand[k] - prev[k] : prev[k] - cand[k];
        if (cost < best_cost) {
            best_cost = cost;
            for (k = 0; k < 5; k++)
                out[k] = cand[k];
        }
    }
}

void tp_harmony(const tp_bank_t *bank, int n, tp_chord_t chords[4], uint8_t kal[64])
{
    const tp_chord_t *orig = bank->chords;
    int tonic = orig[0].root % 12, minor = 0, deg[4], i, k, used[7] = { 0 };
    uint8_t prev[5];
    for (k = 0; k < 5; k++)                      /* minor if the home chord has a minor third */
        if (orig[0].notes[k] % 12 == (tonic + 3) % 12)
            minor = 1;
    tp_bank_kal(bank, kal);
    if (n <= 0) {                                /* 0: the bank's own */
        for (i = 0; i < 4; i++)
            chords[i] = orig[i];
        return;
    }
    hr = bank->seed * 7919u + (uint32_t)n * 104729u + 12345u;
    hrand(1);

    /* the degrees: home first, then the pulls; no repeats, and the last not home (it leads back to it) */
    deg[0] = 0;
    used[0] = 1;
    for (i = 1; i < 4; i++) {
        uint8_t w[7];
        int d;
        for (k = 0; k < 7; k++) {
            w[k] = PULL[minor][deg[i - 1]][k];
            if (used[k])
                w[k] = 0;
            if (i == 3 && (k == 4 || k == 3 || (minor && (k == 6 || k == 5))))
                w[k] = (uint8_t)(w[k] * 2);      /* the turn back home: V, IV (VII, VI in minor) */
        }
        d = pick(w, 7);
        if (d < 0) {                             /* nothing unused pulls: allow a repeat of a non-home chord */
            for (k = 0; k < 7; k++)
                w[k] = k && !(minor ? k == 1 : k == 6) && k != deg[i - 1] ? 1 : 0;
            d = pick(w, 7);
        }
        deg[i] = d;
        used[d] = 1;
    }

    for (k = 0; k < 5; k++)
        prev[k] = orig[0].notes[k];
    {
    int sus = 0;
    for (i = 0; i < 4; i++) {
        int pcs[5], npc = 0, t, j;
        uint8_t w[CT_N];
        for (t = 0; t < CT_N; t++)
            w[t] = type_ok(tonic, minor, deg[i], t) ? STYLE_W[bank->style][t] : 0;
        if (i == 0 || sus)                       /* home is never suspended, and one sus a progression at most: */
            w[CT_SUS2] = w[CT_SUS4] = 0;         /* it has to settle somewhere */
        t = pick(w, CT_N);
        if (t < 0)
            t = type_ok(tonic, minor, deg[i], CT_SEVENTH) && bank->style != TP_STYLE_TRIAD ? CT_SEVENTH : CT_TRIAD;
        if (t == CT_SUS2 || t == CT_SUS4)
            sus = 1;
        for (j = 0; j < 5 && CT_DEG[t][j] >= 0; j++)
            pcs[npc++] = deg_pc(tonic, minor, deg[i], CT_DEG[t][j]);
        chord_name(tonic, minor, deg[i], t, chords[i].name);
        voice(pcs, npc, prev, chords[i].notes);
        for (k = 0; k < 5; k++)
            prev[k] = chords[i].notes[k];
        {   /* the bass: the root, between 33 and 44 (A1 .. G#2), as the banks' own */
            int r = deg_pc(tonic, minor, deg[i], 0);
            chords[i].root = (uint8_t)(33 + interval(9, r));
        }
    }
    }

    /* the note grid: the same rhythm; a note not in the chord under it moves to the nearest scale note that is
     * (from the bank's own pentatonic notes first, then any chord tone near it) */
    for (i = 0; i < 64; i++) {
        int m = kal[i], best = -1, bd = 99, c = i / 16, j;
        int in_chord = 0;
        if (!m)
            continue;
        for (k = 0; k < 5; k++)
            if (chords[c].notes[k] % 12 == m % 12)
                in_chord = 1;
        if (in_chord)
            continue;
        for (j = 0; j < bank->npenta; j++) {
            int p = bank->penta[j], d = p > m ? p - m : m - p, ok = 0;
            for (k = 0; k < 5; k++)
                if (chords[c].notes[k] % 12 == p % 12)
                    ok = 1;
            if (ok && d < bd) {
                bd = d;
                best = p;
            }
        }
        if (best < 0 || bd > 4)
            for (j = m - 4; j <= m + 4; j++)
                for (k = 0; k < 5; k++)
                    if (chords[c].notes[k] % 12 == j % 12 && (j > m ? j - m : m - j) < bd) {
                        bd = j > m ? j - m : m - j;
                        best = j;
                    }
        if (best > 0)
            kal[i] = (uint8_t)best;
    }
}
