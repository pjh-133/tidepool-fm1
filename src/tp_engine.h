/* Tidepool on the FM-1: the engine.
 * The web app builds a fresh graph of Web Audio nodes for every note. Here every sound is a slot in
 * a fixed pool of voices (up to TP_NPART oscillators, a noise source, two filters, an envelope, and
 * a few LFOs), each loop mixes its voices into a mono bus, and the buses are panned into the same
 * sends as the web app: a reverb, a tempo delay, a compressor and the master.
 *
 * Threads, as on the device: tp_render() runs in the audio interrupt; the tp_set and tp_toggle calls
 * only write flags it reads at the start of a block. Float only, no libm (tp_math.h). */
#pragma once
#include <stdint.h>

#define TP_SR 44100.0f
#define TP_BLOCK 32                    /* control-rate block: LFOs, filter sweeps, compressor */
#define TP_NLOOP 16
#define TP_LIVE TP_NLOOP                       /* the keys played live (a canvas's recording mode) */
#define TP_CLICK (TP_NLOOP + 1)                /* the count-in and the click */
#define TP_NBUS (TP_NLOOP + 2)
#ifndef TP_BIG
#if defined(TP_POOL) && !defined(PLAT_HOST)
#define TP_BIG __attribute__((section(".pool")))   /* the FM-1 build (fm1.toml): the big buffers in fm1-base's pool */
#else
#define TP_BIG
#endif
#endif
#ifndef TP_MAXVOICE
#define TP_MAXVOICE 192                /* pool capacity (the device build: 32); tp_voice_limit() sets how many may sound */
#endif

typedef struct {
    char name[8];                               /* as the screen shows it: "Dm9" */
    uint8_t root;
    uint8_t notes[5];
} tp_chord_t;

typedef struct {
    const char *name;
    uint8_t fam;
    float pan, rev, dly, fade;
    void (*start)(int loop, int off);           /* continuous sounds, at activation */
    void (*sched)(int loop, int step, int off); /* each 1/16 step while active */
} tp_loop_t;

typedef struct {
    const char *name;
    const tp_loop_t *loops;                     /* TP_NLOOP of them */
    const tp_chord_t *chords;                   /* 4: one per bar of the 64-step cycle */
    const uint8_t *penta;
    int npenta;
    uint32_t seed;
    float dens;
    const char *fam[4];                         /* the rows' names: "Drones", "Textures", ... */
    uint8_t style;                              /* its harmony, for new progressions (tp_harmony.h): TP_STYLE_* */
    uint8_t canvas;                             /* a Blank Canvas: its loops are recorded (tp_user) */
} tp_bank_t;
enum { TP_STYLE_LUSH, TP_STYLE_FOLK, TP_STYLE_TRIAD };

void tp_init(const tp_bank_t *bank, uint32_t seed);
void tp_stop_fast(void);                        /* a bank switch: every loop out in 0.35 s ... */
int tp_quiet(void);                             /* ... until nothing is sounding (the reverb and echo ring on) ... */
void tp_set_bank(const tp_bank_t *bank);        /* ... then the new bank, its loops as designed, from its first chord */
const tp_bank_t *tp_bank_now(void);
void tp_voice_limit(int n);
void tp_bpm(float bpm);
void tp_toggle(int loop, int on);               /* on: joins on the next beat; off: fades out */
void tp_volume(int loop, float v);
void tp_master(float v);                        /* the MASTER pot, 0..1 (the web app's main volume: 0.8) */
/* each loop's settings, as the web app's loop tweaks: volume 0..1, attack 0..1, decay 0..1 (0.5 = as designed),
 * reverb and echo sends 0..1, pan -1..1. Attack and decay apply to notes that start after the change. */
enum { TP_P_VOL, TP_P_ATT, TP_P_DEC, TP_P_REV, TP_P_ECHO, TP_P_PAN, TP_NPARAM };
void tp_param(int loop, int k, float v);
float tp_param_default(int loop, int k);
int tp_loop_state(int loop);                    /* bit 0 playing, bit 1 joining on the next beat, bit 2 sounding */
float tp_loop_level(int loop);                  /* mean square of its output (TP_METERS builds only) */
void tp_render(float *out_lr, int n);           /* n stereo frames, a multiple of TP_BLOCK */
int tp_shed(int n);                             /* overload: the n quietest notes fade out in 3 ms (render context) */

/* TP_PROFILE builds: the render's stages timed with the app's clock (tp_prof_clock), accumulated in its ticks */
enum { TP_PROF_VOICES, TP_PROF_MIX, TP_PROF_FX, TP_PROF_COMP, TP_PROF_N };
extern uint32_t (*tp_prof_clock)(void);
extern uint32_t tp_prof[TP_PROF_N];

typedef struct {
    int sounding, peak;                         /* voices sounding now, the most so far */
    uint32_t started, stolen, dropped;          /* notes started, stolen from, refused */
    uint32_t shed;                              /* notes faded out early by tp_shed (overload) */
    uint32_t blocks, voice_blocks;              /* for the mean */
} tp_stats_t;
extern tp_stats_t tp_stats;

/* for the bank's loops */
extern int tp_step;
float tp_sps(void);                             /* seconds per step */
float tp_rand(void);                            /* [0, 1) */
const tp_chord_t *tp_chord_at(int step);
const tp_chord_t *tp_chords(void);              /* the four playing */
/* new chords and note grid (tp_harmony): at the next bar line, or now (when nothing plays); while one change waits
 * for its bar line another is ignored (tp_chords_pending: try again) */
void tp_set_chords(const tp_chord_t *chords4, const uint8_t *kal64, int now);
int tp_chords_pending(void);
void tp_bank_kal(const tp_bank_t *b, uint8_t *kal64);   /* the bank's own note grid */
extern uint8_t tp_kal[64];

/* the preset banks (banks.c): the web app's five, in its order, then the FM-1's five */
#define TP_NBANK 12
extern const tp_bank_t *const TP_BANKS[TP_NBANK];
