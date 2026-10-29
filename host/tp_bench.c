/* Tidepool on the FM-1: the host bench. Renders the engine on the Mac, writes WAVs, counts voices and
 * the instructions per sample (kernel-counted, as Felucca's tests/regress.c), and turns those into a
 * device estimate with Felucca's calibration: 1.7 % of the FM-1 per 100 host instructions per sample
 * (tests/drum_test.c, tests/fm6_test.c: "the PHYS measurements' ratio").
 *
 *   tp_bench all SECONDS LIMIT OUT.wav   every loop on from the start
 *   tp_bench solo                        each loop alone: its cost and its voices
 *   tp_bench build                       loops join one by one, as someone playing: voices needed at each count
 *   tp_bench sweep                       all loops, at voice limits 8 .. 64: what is stolen, what it costs
 *   tp_bench tweak                       each loop setting at its extremes, on every loop
 *   tp_bench rvcal                       the reverb's level against the web app's convolver */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <time.h>
#ifdef __APPLE__
#include <libproc.h>
#include <sys/resource.h>
#include <unistd.h>
#endif
#include "../src/tp_engine.h"
#include "../src/tp_harmony.h"

static const tp_bank_t *bank_sel;            /* TP_BANK=1..12 in the environment (default 1, Shallows) */
extern float tp_rv_gain;
#define DEVICE_PCT_PER_INSN 0.017      /* % of the FM-1 per host instruction per sample */

static uint64_t insns(void)
{
#ifdef __APPLE__
    struct rusage_info_v4 ri;
    if (!proc_pid_rusage(getpid(), RUSAGE_INFO_V4, (rusage_info_t *)&ri))
        return ri.ri_instructions;
#endif
    return 0;
}
static double now_s(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

static void wav_write(const char *path, const float *lr, long frames)
{
    FILE *f = fopen(path, "wb");
    uint32_t sr = 44100, bytes = (uint32_t)(frames * 4), u32;
    uint16_t u16;
    long i;
    if (!f) {
        perror(path);
        return;
    }
    fwrite("RIFF", 1, 4, f); u32 = 36 + bytes; fwrite(&u32, 4, 1, f);
    fwrite("WAVEfmt ", 1, 8, f); u32 = 16; fwrite(&u32, 4, 1, f);
    u16 = 1; fwrite(&u16, 2, 1, f); u16 = 2; fwrite(&u16, 2, 1, f);
    fwrite(&sr, 4, 1, f); u32 = sr * 4; fwrite(&u32, 4, 1, f);
    u16 = 4; fwrite(&u16, 2, 1, f); u16 = 16; fwrite(&u16, 2, 1, f);
    fwrite("data", 1, 4, f); fwrite(&bytes, 4, 1, f);
    for (i = 0; i < frames * 2; i++) {
        float x = lr[i] * 32767.0f;
        int16_t s = (int16_t)(x > 32767.0f ? 32767 : x < -32768.0f ? -32768 : x);
        fwrite(&s, 2, 1, f);
    }
    fclose(f);
}

typedef struct {
    double ips, ns, rms, peak_db;
    int peak_v;
    double mean_v;
    uint32_t stolen, dropped, started;
} run_t;

/* render `secs` with the loops in `mask` on (joining at the start), the first `warm` seconds not measured */
static int tweak_k = -1;
static float tweak_v;
static run_t run(uint32_t mask, double secs, int limit, double warm, float *keep)
{
    static float buf[2 * 1024];
    long total = (long)(secs * 44100.0) / 1024 * 1024, warm_n = (long)(warm * 44100.0) / 1024 * 1024, i;
    double sum2 = 0, pk = 0, t0 = 0, t1;
    uint64_t c0 = 0, c1;
    run_t r;
    int l;
    memset(&r, 0, sizeof r);
    tp_init(bank_sel, 1);
    tp_voice_limit(limit);
    if (getenv("TP_CHORDS")) {               /* progression n of the bank (tp_harmony), from the start */
        tp_chord_t c[4];
        uint8_t k[64];
        tp_harmony(bank_sel, atoi(getenv("TP_CHORDS")), c, k);
        tp_set_chords(c, k, 1);
    }
    for (l = 0; l < TP_NLOOP; l++) {
        if (tweak_k >= 0)
            tp_param(l, tweak_k, tweak_v);
        if (mask & (1u << l))
            tp_toggle(l, 1);
    }
    for (i = 0; i < total; i += 1024) {
        if (i == warm_n) {
            tp_stats.peak = 0;
            tp_stats.blocks = tp_stats.voice_blocks = 0;
            c0 = insns();
            t0 = now_s();
        }
        tp_render(buf, 1024);
        if (keep)
            memcpy(keep + 2 * i, buf, sizeof buf);
        if (i >= warm_n) {
            int k;
            for (k = 0; k < 2048; k++) {
                sum2 += (double)buf[k] * buf[k];
                if (fabs(buf[k]) > pk)
                    pk = fabs(buf[k]);
            }
        }
    }
    c1 = insns();
    t1 = now_s();
    r.ips = (double)(c1 - c0) / (double)(total - warm_n);
    r.ns = (t1 - t0) * 1e9 / (double)(total - warm_n);
    r.rms = sqrt(sum2 / (2.0 * (total - warm_n)));
    r.peak_db = 20 * log10(pk + 1e-12);
    r.peak_v = tp_stats.peak;
    r.mean_v = tp_stats.blocks ? (double)tp_stats.voice_blocks / tp_stats.blocks : 0;
    r.stolen = tp_stats.stolen;
    r.dropped = tp_stats.dropped;
    r.started = tp_stats.started;
    return r;
}


int main(int argc, char **argv)
{
    const char *bs = getenv("TP_BANK");
    int bi = bs ? atoi(bs) : 1;
    bank_sel = TP_BANKS[(bi < 1 || bi > TP_NBANK ? 1 : bi) - 1];
    fprintf(stderr, "bank: %s\n", bank_sel->name);
    const char *mode = argc > 1 ? argv[1] : "all";
    if (!strcmp(mode, "all")) {
        double secs = argc > 2 ? atof(argv[2]) : 60;
        int limit = argc > 3 ? atoi(argv[3]) : TP_MAXVOICE;
        const char *out = argc > 4 ? argv[4] : 0;
        long frames = (long)(secs * 44100.0) / 1024 * 1024;
        float *keep = out ? calloc((size_t)frames * 2, sizeof(float)) : 0;
        run_t r = run(0xFFFF, secs, limit, 10, keep);
        printf("all 16 loops, %.0f s, voice limit %d\n", secs, limit);
        printf("  voices: peak %d, mean %.1f; notes %u, stolen %u, refused %u\n", r.peak_v, r.mean_v, r.started,
               r.stolen, r.dropped);
        printf("  level: rms %.1f dBFS, peak %.1f dBFS\n", 20 * log10(r.rms + 1e-12), r.peak_db);
        printf("  cost: %.0f host instructions per sample, %.0f ns per sample -> FM-1 estimate %.0f %%\n", r.ips, r.ns,
               r.ips * DEVICE_PCT_PER_INSN);
        if (out) {
            wav_write(out, keep, frames);
            printf("  wrote %s\n", out);
        }
        free(keep);
    } else if (!strcmp(mode, "mask")) {   /* tp_bench mask HEX SECONDS OUT.wav: these loops, from the start */
        uint32_t mask = argc > 2 ? (uint32_t)strtoul(argv[2], 0, 16) : 0xFFFF;
        double secs = argc > 3 ? atof(argv[3]) : 30;
        long frames = (long)(secs * 44100.0) / 1024 * 1024;
        float *keep = calloc((size_t)frames * 2, sizeof(float));
        run_t r = run(mask, secs, TP_MAXVOICE, 0, keep);
        printf("mask %04X: peak %d voices, rms %.1f dBFS\n", mask, r.peak_v, 20 * log10(r.rms + 1e-12));
        if (argc > 4)
            wav_write(argv[4], keep, frames);
        free(keep);
    } else if (!strcmp(mode, "tweak")) {  /* every loop's setting at the extremes: what it does to voices, level, cost */
        static const char *const NAME[TP_NPARAM] = { "volume", "attack", "decay", "reverb", "echo", "pan" };
        static const float VAL[TP_NPARAM][2] = { { 0.3f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 1.0f }, { -1.0f, 1.0f } };
        int k, j;
        printf("%-8s %6s %6s %6s %9s %9s\n", "setting", "value", "peak", "mean", "rms dBFS", "FM-1 %");
        for (k = 0; k < TP_NPARAM; k++)
            for (j = 0; j < 2; j++) {
                run_t r;
                tweak_k = k;
                tweak_v = VAL[k][j];
                r = run(0xFFFF, 50, TP_MAXVOICE, 10, 0);
                printf("%-8s %6.2f %6d %6.1f %9.1f %8.0f%%\n", NAME[k], tweak_v, r.peak_v, r.mean_v, 20 * log10(r.rms + 1e-12),
                       r.ips * DEVICE_PCT_PER_INSN);
            }
        tweak_k = -1;
    } else if (!strcmp(mode, "chords")) {   /* tp_bench chords [N]: each bank's progressions 0..N, checked */
        int nmax = argc > 2 ? atoi(argv[2]) : 8, b, v, i, k, bad = 0;
        for (b = 0; b < TP_NBANK; b++) {
            const tp_bank_t *B = TP_BANKS[b];
            int tonic = B->chords[0].root % 12, minor = 0, inkey[12] = { 0 };
            static const int SC[2][7] = { { 0, 2, 4, 5, 7, 9, 11 }, { 0, 2, 3, 5, 7, 8, 10 } };
            uint8_t k0[64];
            for (k = 0; k < 5; k++)
                if (B->chords[0].notes[k] % 12 == (tonic + 3) % 12)
                    minor = 1;
            for (k = 0; k < 7; k++)
                inkey[(tonic + SC[minor][k]) % 12] = 1;
            tp_bank_kal(B, k0);
            printf("%s (%s %s)\n", B->name, (const char *[]){ "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" }[tonic],
                   minor ? "minor" : "major");
            for (v = 0; v <= nmax; v++) {
                tp_chord_t c[4];
                uint8_t kal[64];
                int moved = 0, lead = 0;
                tp_harmony(B, v, c, kal);
                for (i = 0; i < 64; i++)
                    moved += kal[i] != k0[i];
                printf("  %2d  ", v);
                for (i = 0; i < 4; i++) {
                    printf("%-7s ", c[i].name);
                    for (k = 0; k < 5; k++)
                        if (!inkey[c[i].notes[k] % 12] && v)
                            bad++;
                    if (i)
                        for (k = 0; k < 5; k++)
                            lead += abs((int)c[i].notes[k] - (int)c[i - 1].notes[k]);
                }
                printf(" | voicing moves %2d semitones | grid notes moved %2d | ", lead, moved);
                for (k = 0; k < 5; k++)
                    printf("%d%s", c[1].notes[k], k < 4 ? "," : "\n");
            }
        }
        printf("notes outside the key: %d\n", bad);
    } else if (!strcmp(mode, "solo")) {
        run_t idle = run(0, 40, TP_MAXVOICE, 10, 0);
        int l;
        printf("%-14s %9s %8s %6s %6s\n", "loop", "insn/smp", "FM-1 %", "peak", "mean");
        printf("%-14s %9.0f %7.1f%% %6d %6.1f\n", "(idle: fx)", idle.ips, idle.ips * DEVICE_PCT_PER_INSN, 0, 0.0);
        for (l = 0; l < TP_NLOOP; l++) {
            run_t r = run(1u << l, 40, TP_MAXVOICE, 10, 0);
            printf("%-14s %9.0f %7.1f%% %6d %6.1f\n", bank_sel->loops[l].name, r.ips - idle.ips,
                   (r.ips - idle.ips) * DEVICE_PCT_PER_INSN, r.peak_v, r.mean_v);
        }
    } else if (!strcmp(mode, "build")) {
        /* the order someone might bring them in: a bed, then texture, then tune, then beat */
        static const int order[TP_NLOOP] = { 0, 1, 4, 9, 12, 13, 10, 2, 7, 8, 15, 3, 5, 11, 14, 6 };
        uint32_t mask = 0;
        int k;
        printf("%-3s %-14s %6s %6s %9s %8s\n", "n", "+ loop", "peak", "mean", "insn/smp", "FM-1 %");
        for (k = 0; k < TP_NLOOP; k++) {
            run_t r;
            mask |= 1u << order[k];
            r = run(mask, 40, TP_MAXVOICE, 10, 0);
            printf("%-3d %-14s %6d %6.1f %9.0f %7.0f%%\n", k + 1, bank_sel->loops[order[k]].name, r.peak_v, r.mean_v,
                   r.ips, r.ips * DEVICE_PCT_PER_INSN);
        }
    } else if (!strcmp(mode, "sweep")) {
        static const int lim[] = { 8, 12, 16, 24, 32, 48, 64, TP_MAXVOICE };
        unsigned k;
        printf("%-6s %6s %8s %8s %9s %8s\n", "limit", "notes", "stolen", "stolen%", "insn/smp", "FM-1 %");
        for (k = 0; k < sizeof lim / sizeof lim[0]; k++) {
            run_t r = run(0xFFFF, 70, lim[k], 10, 0);
            printf("%-6d %6u %8u %7.1f%% %9.0f %7.0f%%\n", lim[k], r.started, r.stolen,
                   100.0 * r.stolen / (r.started ? r.started : 1), r.ips, r.ips * DEVICE_PCT_PER_INSN);
        }
    } else if (!strcmp(mode, "rvcal")) {
        /* the web convolver (normalize on, Chromium's calibration: 0.00125 / rms of the impulse) passes
         * white noise at sqrt(sum h^2) = 0.557 x per channel, before the app's 0.9 */
        extern void tp_reverb_probe(float, float, float *, float *);
        double si = 0, so = 0;
        long i;
        tp_init(bank_sel, 1);
        for (i = 0; i < 44100L * 30; i++) {
            float a = tp_rand() * 2 - 1, b = tp_rand() * 2 - 1, ol, or_;
            tp_reverb_probe(a * 0.1f, b * 0.1f, &ol, &or_);
            if (i > 44100L * 8) {
                si += (double)a * a * 0.01 + (double)b * b * 0.01;
                so += (double)ol * ol + (double)or_ * or_;
            }
        }
        printf("reverb: gain %.4f passes %.3f x; for 0.557 x set tp_rv_gain = %.4f\n", tp_rv_gain, sqrt(so / si),
               tp_rv_gain * 0.557 / sqrt(so / si));
    } else {
        fprintf(stderr, "modes: all solo build sweep rvcal\n");
        return 1;
    }
    return 0;
}
