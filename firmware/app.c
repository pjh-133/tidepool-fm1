/* Tidepool on the FM-1: the app. The thirteen preset banks (the web app's five, then Aurora, Dunes, Zen Garden,
 * Clockwork, Night Cafe, Bonfire, Old Blood and Cyberpunk) and the Blank Canvas on the engine in ../src, played from the panel the way the browser emulator
 * (../web) plays it:
 *   white keys 1-16  the loops, in rows of four (Drones, Melodies, Beats, then Textures: the scene's treatments,
 *                    the banks' second family, kept last): lit playing, blinking joining
 *   KNOB1-4          the selected setting of the selected row's four loops
 *   OCT- / OCT+      the setting: volume, attack, decay, reverb, echo, pan
 *   ALGORITHM        the row (a key pressed picks its row too)
 *   SELECT           tempo;  MASTER  volume;  PRESET  the bank (it changes once the knob rests: everything fades out
 *                    quickly, as the web app's bank switch, and the new bank starts on its first chord)
 *   PLAY             start: the clock waits at bar 1 at power-on, after a bank change and after a stop, so the loops
 *                    picked (blinking, PLAY too) start together; playing, stop everything (the clock back to bar 1);
 *                    stopped with none picked, the mix last stopped (on release; a hold long enough to show the
 *                    chord hint does none of this). SEQ, a surprise and arming a take start the clock too
 *   PLAY + KNOB1     held, turned: the bank's chord progression (0 its own, 1..99 new ones in its key: tp_harmony),
 *                    from the next bar line; the loops that play chords or the note grid follow
 *   HOME             held 3 s (a countdown on the screen): surprise me
 *   SEQ              autoplay, on any bank: loops brought in and out once a cycle (four bars, so faster at a higher
 *                    tempo), something from each row kept playing; the presets' chords change now and then. SEQ
 *                    again: the ending, fewer loops each cycle until all have stopped (SEQ blinking; SEQ once
 *                    more: autoplay again). PLAY stops everything at once
 *   SAVE             the tempo, the bank, and every loop's settings in every bank into the flash (lit: changed since)
 * The Blank Canvas (the last bank) plays loops recorded into it (../src/tp_canvas.h). Its two modes:
 *   playback         as a preset bank, its empty keys saying so; the settings add INSTRUMENT (the loop's)
 *   recording (GLO)  the keys play the instrument (velocity: lib/touch), OCT-/OCT+ their octave, PRESET the
 *                    instrument, KNOB1-3 a new loop's length, snap (off, 1/16, 1/8, bar) and click; PLAY stops and starts the mix.
 *                    REC: a new loop in the next free slot; REC again: a bar's count-in, then it records, round and
 *                    round its length (each pass adds); REC again: kept, saved at once, and playing on (the first
 *                    loop too, ready for the next). GLO during a take cancels it; HOME leaves recording mode as GLO does
 *   EDIT held        the loops: a key picks one to delete, the same key again deletes it, any other key cancels
 * The footer shows the FM-1's own audio load (plat_cpu_pct) and any dropouts. Each bank has its own look (art.c);
 * the start shows a splash for a moment (any key, button or knob goes straight on).
 * Part of fm1-base's unity build (base/device/unity.c, base/host/plat_host.c), after the platform: gfx.c and
 * announce.c are in scope. The engine is its own -O2 unit (fm1.toml o2); only its interface is used here. */
#include "fastmath.h"
#include "../src/tp_engine.h"
#include "../src/tp_harmony.h"
#include "../src/tp_canvas.h"
#include "art.c"                         /* the pixel font, the splash, each bank's theme and banner */

/* ------------------------------------------------------------- settings --- */
/* each loop setting as its knob position u, 0..100 (2 a detent), as the web app's sliders */
enum { S_VOL, S_ATT, S_DEC, S_REV, S_ECHO, S_PAN, NSET, S_INSTR = NSET };   /* S_INSTR: the canvas's (tp_user) */
static const char *const SET_NAME[NSET + 1] = {"VOLUME", "ATTACK", "DECAY", "REVERB", "ECHO", "PAN", "INSTRUMENT"};
static const char *const BTN_NAME[NB] = {"FX", "SEL", "ENV", "LFO", "EDIT", "GLO", "HOME", "SAVE", "ARP", "SEQ",
                                         "PLAY", "REC", "OCT-", "OCT+"};

#define NBK (TP_NBANK + 1)               /* the preset banks, then the Blank Canvas */
#define CANVAS_B TP_NBANK
#define SET_MAGIC 0x38504454u            /* "TDP8": the settings record, thirteen preset banks; the older ones (below)
                                          * are read and kept */
#define SET_MAGIC7 0x37504454u           /* twelve preset banks and the canvas */
#define SET_MAGIC6 0x36504454u           /* ten preset banks and the canvas */
#define SET_MAGIC5 0x35504454u           /* five preset banks and the canvas */
#define SET_MAGIC4 0x34504454u           /* the same, from before the textures were quieter */
#define SET_MAGIC3 0x33504454u
#define SET_MAGIC2 0x32504454u           /* (TDP1, one bank, loads defaults) */
typedef struct {
    uint32_t magic;
    uint8_t bpm;                         /* 56 .. 140 */
    uint8_t bank;                        /* the bank playing, 0 .. NBK-1 */
    uint8_t rsv[2];
    uint8_t u[NBK][TP_NLOOP][NSET];      /* each bank's loops' settings, knob positions 0..100 */
    uint8_t prog[NBK];                   /* each bank's chord progression (TDP3): 0 its own */
    uint8_t instr, bars, snap, click;    /* the canvas's recorder (TDP4): the keys' instrument, a new loop's length
                                          * (1, 2, 4 bars), snap (TP_SNAP_*: off, 1/16, 1/8, bar), the click */
    int8_t oct;                          /* the keys' octave, -2 .. 2 */
} settings_t;
/* the older records, as they were: five preset banks, then (TDP4, TDP5) the canvas; TDP3 and TDP2 the five banks'
 * settings and (TDP3) their progressions */
#define OLD_NBANK 5
typedef struct {
    uint32_t magic;
    uint8_t bpm, bank, rsv[2];
    uint8_t u[OLD_NBANK + 1][TP_NLOOP][NSET];
    uint8_t prog[OLD_NBANK + 1];
    uint8_t instr, bars, snap, click;
    int8_t oct;
} settings5_t;
#define OLD6_NBANK 10                    /* TDP6: ten preset banks, then the canvas */
typedef struct {
    uint32_t magic;
    uint8_t bpm, bank, rsv[2];
    uint8_t u[OLD6_NBANK + 1][TP_NLOOP][NSET];
    uint8_t prog[OLD6_NBANK + 1];
    uint8_t instr, bars, snap, click;
    int8_t oct;
} settings6_t;
#define OLD7_NBANK 12                    /* TDP7: twelve preset banks, then the canvas */
typedef struct {
    uint32_t magic;
    uint8_t bpm, bank, rsv[2];
    uint8_t u[OLD7_NBANK + 1][TP_NLOOP][NSET];
    uint8_t prog[OLD7_NBANK + 1];
    uint8_t instr, bars, snap, click;
    int8_t oct;
} settings7_t;
#define SET_U_OFF __builtin_offsetof(settings_t, u)
#define SET2_SIZE (SET_U_OFF + OLD_NBANK * TP_NLOOP * NSET)
#define SET3_SIZE ((SET2_SIZE + OLD_NBANK + 3u) & ~3u)
static settings_t set;
static uint8_t set_dirty;                /* changed since the last save: SAVE is lit */

static float u_value(int k, int32_t u)   /* knob position -> the engine's value */
{
    float x = (float)u / 100.0f;
    if (k == S_PAN) {
        float y = x * 2.0f - 1.0f;
        return y > -0.06f && y < 0.06f ? 0.0f : y;   /* a sticky centre */
    }
    return x;
}

static int32_t value_u(int k, float v)   /* the engine's value -> knob position */
{
    float x = k == S_PAN ? (v + 1.0f) * 0.5f : v;
    return clamp((int32_t)(x * 100.0f + 0.5f), 0, 100);
}

static const tp_bank_t *bank_of(int b) { return b < TP_NBANK ? TP_BANKS[b] : &TP_CANVAS; }
#define BANK (bank_of(set.bank))
#define IS_CANVAS (set.bank == CANVAS_B)
#define USET (set.u[set.bank])              /* the playing bank's settings */
static int nset(void) { return IS_CANVAS ? NSET + 1 : NSET; }   /* the settings OCT-/OCT+ cycle */

static void apply_loop(int l)
{
    int k;
    for (k = 0; k < NSET; k++)
        tp_param(l, k, u_value(k, USET[l][k]));
}

/* the bank's chord progression (set.prog): now (nothing plays), or from the next bar line. 0 if a change is still
 * waiting for its bar line (try again) */
static int apply_progression(int now)
{
    tp_chord_t c[4];
    uint8_t kal[64];
    if (IS_CANVAS)                           /* its loops play their own notes */
        return 1;
    if (!now && tp_chords_pending())
        return 0;
    tp_harmony(TP_BANKS[set.bank], set.prog[set.bank], c, kal);
    tp_set_chords(c, kal, now);
    return 1;
}

/* a preset bank's second family: its textures (Textures, Atmosphere, Woods, City, Depths), the scene's treatments
 * rather than an instrument: quieter, last on the keys, one at a time and seldom changed in autoplay */
#define FAM_TEX 1
#define TEX_VOL 25                       /* their volume, knob position */
static int is_tex(int b, int l) { return b < TP_NBANK && l / 4 == FAM_TEX; }

static float bank_default(int b, int l, int k)   /* a loop's settings as designed (the engine's tp_param_default) */
{
    const tp_loop_t *L = &bank_of(b)->loops[l];
    switch (k) {
    case S_VOL: return is_tex(b, l) ? (float)TEX_VOL / 100.0f : 0.7f;
    case S_ATT: return 0.0f;
    case S_DEC: return 0.5f;
    case S_REV: return L->rev;
    case S_ECHO: return L->dly;
    default: return L->pan;
    }
}

static void bank_defaults(int b)
{
    int l, k;
    set.prog[b] = 0;
    for (l = 0; l < TP_NLOOP; l++)
        for (k = 0; k < NSET; k++)
            set.u[b][l][k] = (uint8_t)value_u(k, bank_default(b, l, k));
}

static void recorder_defaults(void)
{
    set.instr = 0;
    set.bars = 2;
    set.snap = 1;
    set.click = 1;
    set.oct = 0;
}

static void settings_defaults(void)
{
    int b;
    memset(&set, 0, sizeof set);
    set.magic = SET_MAGIC;
    set.bpm = 72;
    set.bank = 0;
    for (b = 0; b < NBK; b++)
        bank_defaults(b);
    recorder_defaults();
}

/* an older record with n preset banks, then the canvas: its banks' settings and progressions, the canvas's (now after
 * all the preset banks) and the recorder's; the newer banks their defaults */
#define FROM_OLDER(o, n)                                                                                            \
    do {                                                                                                           \
        settings_defaults();                                                                                       \
        set.bpm = (o)->bpm;                                                                                        \
        set.bank = (uint8_t)((o)->bank == (n) ? CANVAS_B : (o)->bank);                                             \
        for (b = 0; b < (n); b++) {                                                                                \
            memcpy(set.u[b], (o)->u[b], sizeof set.u[b]);                                                          \
            set.prog[b] = (o)->prog[b];                                                                            \
        }                                                                                                          \
        memcpy(set.u[CANVAS_B], (o)->u[n], sizeof set.u[CANVAS_B]);                                                \
        set.prog[CANVAS_B] = (o)->prog[n];                                                                         \
        set.instr = (o)->instr;                                                                                    \
        set.bars = (o)->bars;                                                                                      \
        set.snap = (o)->snap;                                                                                      \
        set.click = (o)->click;                                                                                    \
        set.oct = (o)->oct;                                                                                        \
    } while (0)

/* the settings from the flash: TDP8, or an older record kept (what it has carried over, the rest defaults), or the
 * defaults */
static void settings_load(void)
{
    static union {
        settings_t s;
        settings7_t s7;
        settings6_t s6;
        settings5_t s5;
        uint8_t raw[sizeof(settings_t)];
    } ld;
    int got = plat_store_load(OBJ_SETTINGS, &ld, sizeof ld), b, l, older = 0;
    uint32_t magic = ld.s.magic;
    if (got == (int)sizeof(settings_t) && magic == SET_MAGIC)
        set = ld.s;
    else if (got == (int)sizeof(settings7_t) && magic == SET_MAGIC7)
        FROM_OLDER(&ld.s7, OLD7_NBANK);
    else if (got == (int)sizeof(settings6_t) && magic == SET_MAGIC6)
        FROM_OLDER(&ld.s6, OLD6_NBANK);
    else if (got == (int)sizeof(settings5_t) && (magic == SET_MAGIC5 || magic == SET_MAGIC4)) {
        FROM_OLDER(&ld.s5, OLD_NBANK);
        older = magic == SET_MAGIC4;
    } else if ((got == (int)SET2_SIZE && magic == SET_MAGIC2) || (got == (int)SET3_SIZE && magic == SET_MAGIC3)) {
        settings_defaults();                 /* the five banks' settings, and (TDP3) their progressions */
        set.bpm = ld.raw[4];
        set.bank = ld.raw[5] < OLD_NBANK ? ld.raw[5] : 0;
        memcpy(set.u, ld.raw + SET_U_OFF, OLD_NBANK * TP_NLOOP * NSET);
        for (b = 0; b < OLD_NBANK; b++)
            set.prog[b] = got == (int)SET3_SIZE ? ld.raw[SET2_SIZE + (uint32_t)b] : 0;
        older = 1;
    } else
        settings_defaults();
    if (older)                               /* from before: textures left at the old default go quieter */
        for (b = 0; b < OLD_NBANK; b++)
            for (l = FAM_TEX * 4; l < FAM_TEX * 4 + 4; l++)
                if (set.u[b][l][S_VOL] == 70)
                    set.u[b][l][S_VOL] = TEX_VOL;
    set.magic = SET_MAGIC;
    if (set.bpm < 56 || set.bpm > 140 || set.bank >= NBK)
        settings_defaults();
    for (b = 0; b < NBK; b++)
        if (set.prog[b] > TP_HARMONY_MAX)
            set.prog[b] = 0;
    if (set.instr >= TP_NINSTR || (set.bars != 1 && set.bars != 2 && set.bars != 4) || set.snap >= TP_NSNAP ||
        set.click > 1 || set.oct < -2 || set.oct > 2)
        recorder_defaults();
}

/* ------------------------------------------------------- the canvas's loops --- */
/* a loop an object: its header and its notes (only those it has), saved as it is recorded or deleted */
#define CV_MAGIC 0x31564354u             /* "TCV1" */
typedef struct {
    uint32_t magic;
    tp_user_t u;
} canvas_obj_t;
#define CV_HEAD (__builtin_offsetof(canvas_obj_t, u) + __builtin_offsetof(tp_user_t, notes))
static canvas_obj_t cobj;                /* a loop staged for the flash, both ways */
static uint16_t cv_dirty;                /* loops whose instrument changed since the last SAVE */

static int canvas_save(int l)
{
    uint32_t len;
    cobj.magic = CV_MAGIC;
    cobj.u = tp_user[l];
    if (!cobj.u.used)
        cobj.u.n = 0;
    len = CV_HEAD + (uint32_t)cobj.u.n * sizeof(tp_note_t);
    if (plat_store_save(OBJ_CANVAS + (uint32_t)l, &cobj, len))
        return -1;
    cv_dirty &= (uint16_t)~(1u << l);
    return 0;
}

static void canvas_load(void)
{
    int l;
    for (l = 0; l < TP_NLOOP; l++) {
        int got = plat_store_load(OBJ_CANVAS + (uint32_t)l, &cobj, sizeof cobj);
        tp_user_t *U = &tp_user[l];
        U->used = 0;
        U->n = 0;
        U->instr = 0;
        U->bars = 2;
        U->snap = 1;
        if (got >= (int)CV_HEAD && cobj.magic == CV_MAGIC && cobj.u.n <= TP_MAXNOTES &&
            got == (int)(CV_HEAD + (uint32_t)cobj.u.n * sizeof(tp_note_t)) && cobj.u.instr < TP_NINSTR &&
            (cobj.u.bars == 1 || cobj.u.bars == 2 || cobj.u.bars == 4) && cobj.u.snap < TP_NSNAP) {
            *U = cobj.u;
            U->used = cobj.u.used && cobj.u.n;
        }
    }
}

/* -------------------------------------------------------------- UI state --- */
static const uint8_t WK[16] = PLAT_WHITE_KEYS, BK[11] = PLAT_BLACK_KEYS;

/* the rows on the keys and the screen: the presets' families Drones, Melodies, Beats, then Textures (their second,
 * the scene's treatments rather than an instrument) last; the canvas's rows as they are. Settings stay by loop */
static const uint8_t ROW_FAM[4] = {0, 2, 3, 1};
static int row_fam(int r) { return set.bank == TP_NBANK ? r : ROW_FAM[r]; }   /* row r's family */
static int loop_at(int pos) { return row_fam(pos / 4) * 4 + pos % 4; }       /* key or cell pos's loop */
static int pos_of(int l)                     /* loop l's key and cell */
{
    int r;
    for (r = 0; r < 4; r++)
        if (row_fam(r) == l / 4)
            break;
    return r * 4 + l % 4;
}
static const char *const BLACK_NAME[11] = {"F#3", "G#3", "A#3", "C#4", "D#4", "F#4", "G#4", "A#4", "C#5", "D#5",
                                           "F#5"};

static struct {
    char msg[2][24];                     /* a short message over the header */
    uint32_t msg_until;
    uint32_t keys, btn;                  /* held at the last input */
    uint8_t want[TP_NLOOP];              /* the loops asked to play */
    uint16_t last_set;                   /* the mix PLAY stopped: PLAY again brings it back */
    uint8_t row, setting;
    uint8_t pick, switching;             /* PRESET's bank, taken once the knob rests (pick_at); fading out for it */
    uint32_t home_t;                     /* HOME went down (0: not held); surprise at 3 s */
    uint32_t intro_t;                    /* the splash's start (0: over) */
    uint8_t touched;                     /* a key, button or knob since the last frame (ends the splash) */
    uint8_t home_fired, home_left;       /* the surprise came for this hold; the countdown's last number shown */
    uint8_t play_down, play_turned;      /* PLAY held; KNOB1 turned while it was (then its release does nothing) */
    uint8_t play_hint;                   /* PLAY held a moment: KNOB1's chords offered on the screen */
    uint32_t play_t;                     /* PLAY went down */
    uint8_t prog_waiting;                /* a progression chosen, waiting for the last change's bar line */
    uint8_t rec_mode;                    /* the canvas: 0 playback, 1 recording (GLO) */
    uint8_t rs, rslot;                   /* the recorder (R_*), and the loop it is making */
    uint8_t rsaving, rkeep;              /* a take finished: saved once the engine has closed it; it plays on */
    int8_t last_slot;                    /* the loop recorded last, shown in recording mode (-1: none) */
    uint8_t edit_down;                   /* EDIT held: the loops, to delete one */
    int8_t del_sel;                      /* the loop picked to delete (-1: none) */
    uint16_t live[NKEYS];                /* each key's note, playing live (tp_live_on's id; 0 none) */
    uint8_t autoplay;                    /* SEQ: the loops (and the presets' chords) change on their own; 2 ending */
    uint32_t auto_cycle;                 /* the cycle autoplay last acted for */
    uint16_t auto_n;                     /* its cycles since it began */
    uint16_t auto_age[TP_NLOOP];         /* each loop's cycles since autoplay last brought it in or out */
    uint16_t auto_tex_moves, auto_chord_moves;   /* textures swapped, progressions changed (the simulator's checks) */
    uint32_t pick_at;
    uint32_t rnd;
    uint8_t redraw;                      /* bit 0 header, 1 grid, 2 footer */
} ui;

/* ------------------------------------------------------------------ sound --- */
static float fbuf[2 * APP_BLOCK];
static float gain_now;                   /* glides to MASTER: no step at a block's edge */
static volatile uint8_t muted;

/* the render's own time, for the bench over SysEx (app_sysex): its sum and worst block, and the budget it had */
static volatile uint32_t pr_sum, pr_max, pr_budget, pr_blocks;

void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    uint32_t i, t0 = plat_ticks24(), d;
    float gain = (float)master_q12 * (8300000.0f / 4096.0f);   /* the engine's +-1 to 24 bits, at MASTER */
    if (n > APP_BLOCK)
        n = APP_BLOCK;
    tp_render(fbuf, (int)n);             /* n is a whole number of the engine's 32-frame blocks */
    for (i = 0; i < n; i++) {
        float l, r;
        gain_now += (gain - gain_now) * 0.004f;
        l = muted ? 0.0f : fbuf[2 * i] * gain_now;
        r = muted ? 0.0f : fbuf[2 * i + 1] * gain_now;
        lr[2 * i] = (int32_t)(l > 8388607.0f ? 8388607.0f : l < -8388607.0f ? -8388607.0f : l);
        lr[2 * i + 1] = (int32_t)(r > 8388607.0f ? 8388607.0f : r < -8388607.0f ? -8388607.0f : r);
    }
    d = plat_ticks24() - t0;                 /* 24 MHz ticks; a block of n frames has n * 544 (44,117.6 Hz) */
    pr_sum += d;
    if (d > pr_max)
        pr_max = d;
    pr_budget += n * 544u;
    pr_blocks++;
}

void app_panic(void) { muted = 1; }

/* fm1-base: the last block took over 90 % of its time (the audio interrupt, right after the render). The two quietest
 * notes fade out in 3 ms, so the next blocks have room: a shorter tail instead of a dropout. */
void app_overload(void) { tp_shed(2); }

static void ui_reset(void);

void app_init(void)
{
    int l;
    muted = 0;
    ui_reset();                              /* nothing asked to play: a start begins quiet */
    settings_load();
    canvas_load();
    tp_init(BANK, plat_ticks24() | 1u);
    tp_hold(1);                              /* nothing moves until PLAY or SEQ: loops picked first start together */
    apply_progression(1);
    tp_prof_clock = plat_ticks24;            /* the engine's stages, timed (fm1.toml: TP_PROFILE) */
    tp_voice_limit(32);
    for (l = 0; l < TP_NLOOP; l++) {
        int k;
        for (k = 0; k < NSET; k++)
            if (USET[l][k] > 100)
                USET[l][k] = 100;
        apply_loop(l);
    }
    tp_bpm((float)set.bpm);
    ui.pick = set.bank;
}

/* --------------------------------------------------------------------- UI --- */

/* a moment kept as "when it began", 0 meaning none: never in the future (rounding it up to odd made a time 1 ms
 * ahead, and now minus it a wrap past 4 billion: HOME's surprise came at once on an even millisecond) */
static uint32_t stamp(void)
{
    uint32_t t = plat_ms();
    return t ? t : 1u;
}

static void text_cat(char *d, const char *s)
{
    while (*d)
        d++;
    while (*s)
        *d++ = *s++;
    *d = 0;
}

static void int_text(int32_t v, char *b)     /* signed, no padding */
{
    char t[12];
    int n = 0;
    uint32_t u = (uint32_t)(v < 0 ? -v : v);
    do
        t[n++] = (char)('0' + u % 10u);
    while ((u /= 10u) && n < 11);
    if (v < 0)
        *b++ = '-';
    while (n)
        *b++ = t[--n];
    *b = 0;
}

static void tenths_text(int32_t t10, char *b)   /* 12 -> "1.2" */
{
    int_text(t10 / 10, b);
    while (*b)
        b++;
    *b++ = '.';
    *b++ = (char)('0' + t10 % 10);
    *b = 0;
}

static void upper(char *d, const char *s, uint32_t max)
{
    uint32_t i;
    for (i = 0; i + 1 < max && s[i]; i++)
        d[i] = (char)(s[i] >= 'a' && s[i] <= 'z' ? s[i] - 32 : s[i]);
    d[i] = 0;
}

/* a setting's value as the web app shows it */
static void value_text(int k, int32_t u, char *b)
{
    char t[12];
    b[0] = 0;
    if (k == S_ATT) {
        if (!u) {
            text_cat(b, "NATURAL");
            return;
        }
        text_cat(b, "+");
        tenths_text((u * u * 25 + 500) / 1000, t);   /* (u/100)^2 * 2.5 s, in tenths */
        text_cat(b, t);
        text_cat(b, " S");
    } else if (k == S_DEC) {
        if (u == 50) {
            text_cat(b, "NATURAL");
            return;
        }
        tenths_text((int32_t)(fm_exp2f(((float)u / 100.0f - 0.5f) * 4.0f) * 10.0f + 0.5f), t);
        text_cat(b, t);
        text_cat(b, "X");
    } else if (k == S_PAN) {
        float v = u_value(S_PAN, u);
        if (v == 0.0f) {
            text_cat(b, "CENTRE");
            return;
        }
        text_cat(b, v < 0.0f ? "L " : "R ");
        int_text((int32_t)((v < 0.0f ? -v : v) * 100.0f + 0.5f), t);
        text_cat(b, t);
        text_cat(b, "%");
    } else {
        int_text(u, t);
        text_cat(b, t);
        text_cat(b, "%");
    }
}

void app_say(const char *a, const char *b)
{
    uint32_t i;
    char t[48];
    for (i = 0; i < 23u && a && a[i]; i++)
        ui.msg[0][i] = a[i];
    ui.msg[0][i] = 0;
    for (i = 0; i < 23u && b && b[i]; i++)
        ui.msg[1][i] = b[i];
    ui.msg[1][i] = 0;
    ui.msg_until = plat_ms() + 1400u;
    ui.redraw |= 1;
    if (ann_on()) {                          /* and to a screen reader (announce.c, fm1-reader) */
        t[0] = 0;
        text_cat(t, ui.msg[0]);
        if (ui.msg[1][0]) {
            text_cat(t, " ");
            text_cat(t, ui.msg[1]);
        }
        ann_say(t, 0, ANN_NOW);
    }
}

static void loop_name(int l, char *b) { upper(b, BANK->loops[l].name, 16); }
static void fam_name(int r, char *b) { upper(b, BANK->fam[row_fam(r)], 16); }   /* row r's name */
static void bank_name(int i, char *b) { upper(b, bank_of(i)->name, 16); }
static void instr_name(int i, char *b) { upper(b, tp_instr_name(i), 16); }

static void set_loop(int l, int on)
{
    ui.want[l] = (uint8_t)on;
    tp_toggle(l, on);
}

static void toggle(int l)
{
    char n[16];
    if (IS_CANVAS && !tp_user[l].used) {    /* nothing recorded there yet */
        loop_name(l, n);
        app_say(n, "EMPTY - GLO TO RECORD");
        return;
    }
    set_loop(l, !ui.want[l]);
    ui.row = (uint8_t)(pos_of(l) / 4);       /* the knobs follow the row you last touched */
    loop_name(l, n);
    app_say(n, !ui.want[l] ? "FADING OUT" : tp_held() ? "READY - PLAY: START" : "JOINS ON THE BEAT");
    ui.redraw |= 7;
}

static uint32_t rnd(uint32_t n)              /* 0 .. n-1 */
{
    ui.rnd = ui.rnd * 1664525u + 1013904223u;
    return (ui.rnd >> 8) % n;
}

static void surprise(void)                   /* the web app's Surprise me: 1-2 loops a family (0-2 beats) */
{
    static const uint8_t lo[4] = {1, 1, 1, 0}, hi[4] = {2, 2, 2, 2};
    int l, f, played = 0;
    char t[16];
    ui.rnd ^= plat_ticks24();
    for (l = 0; l < TP_NLOOP; l++)
        if (ui.want[l])
            set_loop(l, 0);
    for (f = 0; f < 4; f++) {
        uint8_t order[4] = {0, 1, 2, 3};
        int n = lo[f] + (int)rnd((uint32_t)(hi[f] - lo[f] + 1)), i;
        for (i = 3; i > 0; i--) {            /* shuffle the family */
            int j = (int)rnd((uint32_t)i + 1u);
            uint8_t s = order[i];
            order[i] = order[j];
            order[j] = s;
        }
        for (i = 0; i < n; i++) {
            l = f * 4 + order[i];
            USET[l][S_VOL] = (uint8_t)(is_tex(set.bank, l) ? 15 + rnd(21) : 40 + rnd(51));   /* textures quieter */
            tp_param(l, S_VOL, u_value(S_VOL, USET[l][S_VOL]));
            set_loop(l, 1);
            played++;
        }
    }
    set_dirty = 1;
    int_text(played, t);
    text_cat(t, " LOOPS");
    tp_hold(0);                              /* it plays at once */
    app_say("SURPRISE", t);
}

static void play_button(void)
{
    uint16_t on = 0;
    int l, n = 0;
    char t[16];
    for (l = 0; l < TP_NLOOP; l++)
        if (ui.want[l])
            on |= (uint16_t)(1u << l);
    if (on && !tp_held()) {                  /* playing: everything out, the clock held at bar 1 */
        ui.last_set = on;
        for (l = 0; l < TP_NLOOP; l++)
            if (on >> l & 1u)
                set_loop(l, 0);
        tp_hold(1);
        app_say("STOP", "AGAIN: SAME MIX");
        return;
    }
    if (!on && !ui.last_set && IS_CANVAS)    /* the canvas, nothing chosen or stopped yet: every loop recorded */
        for (l = 0; l < TP_NLOOP; l++)
            if (tp_user[l].used)
                ui.last_set |= (uint16_t)(1u << l);
    if (!on && ui.last_set) {                /* nothing chosen: the mix last stopped */
        for (l = 0; l < TP_NLOOP; l++)
            if (ui.last_set >> l & 1u)
                set_loop(l, 1);
        on = ui.last_set;
    }
    if (!on) {
        app_say("PLAY", IS_CANVAS ? "NOTHING RECORDED YET" : "PICK LOOPS ON THE KEYS");
        return;
    }
    for (l = 0; l < TP_NLOOP; l++)
        n += on >> l & 1u;
    tp_hold(0);                              /* from bar 1, the loops chosen coming in together */
    int_text(n, t);
    text_cat(t, n == 1 ? " LOOP" : " LOOPS");
    app_say("PLAY", t);
}

/* PLAY held a moment (a tap to stop or play shows nothing): KNOB1's chords offered until it is let go */
static void play_hint(uint32_t now)
{
    int want = ui.play_down && !IS_CANVAS && !ui.switching && now - ui.play_t >= 350u;
    if (want && !ui.play_hint && !ui.play_turned) {   /* said once; held on the screen while PLAY is */
        char t[16];
        t[0] = 0;
        if (set.prog[set.bank]) {
            text_cat(t, "NOW NO. ");
            int_text(set.prog[set.bank], t + 8);
        } else
            text_cat(t, "NOW ORIGINAL");
        app_say("KNOB 1: CHORDS", t);
    }
    if (want && !ui.play_turned && ui.msg_until < now + 200u)
        ui.msg_until = now + 400u;
    if (want != ui.play_hint) {
        if (!want && !ui.play_turned && ui.msg_until)   /* let go: the hint goes with it */
            ui.msg_until = now;
        ui.play_hint = (uint8_t)want;
        ui.redraw |= 1;
    }
}

/* PLAY + KNOB1: progression n of the bank, its chords on the screen */
static void chord_knob(int32_t e)
{
    char t[24], b[8];
    tp_chord_t c[4];
    uint8_t kal[64];
    int i;
    set.prog[set.bank] = (uint8_t)clamp((int32_t)set.prog[set.bank] + e, 0, TP_HARMONY_MAX);
    set_dirty = 1;
    ui.prog_waiting = 1;                     /* app_frame applies it (at the next bar line) */
    tp_harmony(TP_BANKS[set.bank], set.prog[set.bank], c, kal);
    t[0] = 0;
    if (set.prog[set.bank]) {
        text_cat(t, "CHORDS ");
        int_text(set.prog[set.bank], b);
        text_cat(t, b);
    } else
        text_cat(t, "CHORDS: ORIGINAL");
    {
        char names[24];
        names[0] = 0;
        for (i = 0; i < 4; i++) {
            if (i)
                text_cat(names, " ");
            text_cat(names, c[i].name);
        }
        names[23] = 0;
        app_say(t, "");
        for (i = 0; i < 23 && names[i]; i++)  /* the chords as the second line (app_say keeps 23 characters) */
            ui.msg[1][i] = names[i];
        ui.msg[1][i] = 0;
    }
}

static void knob(int k, int32_t e)           /* KNOB1-4: the setting of loop k of the row */
{
    int l = loop_at(ui.row * 4 + k);
    int32_t u;
    char n[16], v[16], t[24];
    if (ui.setting == S_INSTR) {             /* the canvas: the loop's instrument */
        tp_user_t *U = &tp_user[l];
        loop_name(l, n);
        if (!U->used) {
            app_say(n, "EMPTY");
            return;
        }
        U->instr = (uint8_t)clamp((int32_t)U->instr + e, 0, TP_NINSTR - 1);
        cv_dirty |= (uint16_t)(1u << l);
        set_dirty = 1;
        instr_name(U->instr, v);
        app_say(n, v);
        ui.redraw |= 2;
        return;
    }
    u = clamp((int32_t)USET[l][ui.setting] + 2 * e, 0, 100);
    USET[l][ui.setting] = (uint8_t)u;
    tp_param(l, ui.setting, u_value(ui.setting, u));
    set_dirty = 1;
    loop_name(l, n);
    t[0] = 0;
    text_cat(t, n);
    text_cat(t, " ");
    text_cat(t, SET_NAME[ui.setting]);
    value_text(ui.setting, u, v);
    app_say(t, v);
    ui.redraw |= 2;
}

/* HOME held: 3, 2, 1 on the screen, then a surprise; let go before, and nothing happens */
static void home_hold(uint32_t btn)
{
    uint32_t held, left;
    char t[4];
    if (!ui.home_t)
        return;
    held = plat_ms() - ui.home_t;
    if (!(btn >> B_HOME & 1u)) {             /* let go */
        if (!ui.home_fired)
            app_say("SURPRISE", "HOLD HOME 3 S");
        ui.home_t = 0;
        ui.redraw |= 1;
        return;
    }
    if (ui.home_fired)
        return;
    if (held >= 3000u) {
        ui.home_fired = 1;
        surprise();
        return;
    }
    left = 3u - held / 1000u;                /* 3, 2, 1 */
    if (left != ui.home_left) {
        ui.home_left = (uint8_t)left;
        t[0] = (char)('0' + left);
        t[1] = 0;
        app_say("SURPRISE IN", t);
    }
}

/* -------------------------------------------------------------- autoplay --- */
/* once a cycle, toward two loops in each row (one texture): a row with nothing playing gets a loop, then a row short
 * of its two gets its second, one a cycle (from nothing, full in seven cycles); full, mostly one loop of a row swapped
 * for another of it, now and then one out for a cycle, sometimes nothing. Never a row left empty; the texture
 * changed only after 64 bars, about one cycle in four after that; the loop
 * playing longest goes first, the one silent longest comes in first. The presets' chords change every four cycles
 * or so (from the next bar line, as PLAY + KNOB1). The canvas: only its recorded loops */
#define AUTO_ROW_MAX 2
#define AUTO_TEX_MIN 16                  /* a texture plays at least this many cycles (64 bars) before it changes, */
#define AUTO_TEX_ODDS 4                  /* then about one cycle in this many: some 20 cycles (4-5 min at 72 bpm) */

static int auto_row_max(int f) { return !IS_CANVAS && f == FAM_TEX ? 1 : AUTO_ROW_MAX; }

static int auto_avail(int l) { return !IS_CANVAS || tp_user[l].used; }

static int auto_pick(int fam, int on)        /* fam's loop to bring in (on 0: off now) or out (on 1) */
{
    int l, best = -1;
    int32_t score = -1;
    for (l = fam * 4; l < fam * 4 + 4; l++)
        if (auto_avail(l) && ui.want[l] == on) {
            int32_t sc = (int32_t)ui.auto_age[l] * 4 + (int32_t)rnd(6);
            if (sc > score) {
                score = sc;
                best = l;
            }
        }
    return best;
}

static void auto_set(int l, int on)
{
    if (l < 0)
        return;
    set_loop(l, on);
    ui.auto_age[l] = 0;
    ui.redraw |= 2;
}

static void auto_chords(void)                /* a new progression for the preset bank, from the next bar line */
{
    uint8_t p = set.prog[set.bank];
    if (IS_CANVAS)
        return;
    while (p == set.prog[set.bank])          /* now and then back to the bank's own */
        p = (uint8_t)(rnd(5) == 0 ? 0 : 1 + rnd(TP_HARMONY_MAX));
    set.prog[set.bank] = p;
    set_dirty = 1;
    ui.prog_waiting = 1;
    ui.auto_chord_moves++;
}

static void auto_step(void)
{
    int l, f, i, nav[4] = {0, 0, 0, 0}, cnt[4] = {0, 0, 0, 0}, r;
    uint8_t order[4] = {0, 1, 2, 3};
    for (l = 0; l < TP_NLOOP; l++) {
        if (ui.auto_age[l] < 0xFFFFu)
            ui.auto_age[l]++;
        if (!auto_avail(l))
            continue;
        nav[l / 4]++;
        if (ui.want[l])
            cnt[l / 4]++;
    }
    ui.auto_n++;
    if (ui.auto_n > 1 && ui.auto_n % 4 == 1 && rnd(4))   /* every fourth cycle, mostly: new chords */
        auto_chords();
    for (i = 3; i > 0; i--) {                /* the rows in a fresh order each time */
        int j = (int)rnd((uint32_t)i + 1u);
        uint8_t t = order[i];
        order[i] = order[j];
        order[j] = t;
    }
    for (i = 0; i < 4; i++) {                /* a row with nothing playing: one of its loops */
        f = order[i];
        if (nav[f] && !cnt[f]) {
            auto_set(auto_pick(f, 0), 1);
            return;
        }
    }
    if (!IS_CANVAS && cnt[FAM_TEX] > 1) {    /* more than one texture (by hand): back to one */
        auto_set(auto_pick(FAM_TEX, 1), 0);
        return;
    }
    if (!IS_CANVAS && cnt[FAM_TEX] == 1 && nav[FAM_TEX] > 1) {   /* the texture: seldom, a swap */
        int ready = 1;
        for (l = FAM_TEX * 4; l < FAM_TEX * 4 + 4; l++)
            if (ui.want[l] && ui.auto_age[l] < AUTO_TEX_MIN)
                ready = 0;
        if (ready && rnd(AUTO_TEX_ODDS) == 0) {
            int in = auto_pick(FAM_TEX, 0), out = auto_pick(FAM_TEX, 1);
            auto_set(out, 0);
            auto_set(in, 1);
            ui.auto_tex_moves++;
            return;
        }
    }
    for (i = 0; i < 4; i++) {                /* building up: a row short of its two (the texture: one) */
        f = order[i];
        if (cnt[f] < auto_row_max(f) && nav[f] > cnt[f]) {
            auto_set(auto_pick(f, 0), 1);
            return;
        }
    }
    r = (int)rnd(100);                       /* full: mostly a swap, now and then one out (back next cycle) */
    for (i = 0; i < 4; i++) {
        f = order[i];
        if (!IS_CANVAS && f == FAM_TEX)      /* the texture changes only as above */
            continue;
        if (r < 70 && cnt[f] && nav[f] > cnt[f]) {   /* a swap within the row: the old out, a new one in */
            int in = auto_pick(f, 0), out = auto_pick(f, 1);   /* both chosen first: not the same loop back */
            auto_set(out, 0);
            auto_set(in, 1);
            return;
        }
        if (r >= 70 && r < 85 && cnt[f] > 1) {   /* one out for a cycle (never a row's last) */
            auto_set(auto_pick(f, 1), 0);
            return;
        }
    }                                        /* else: this cycle, nothing */
}

/* SEQ while autoplaying: the ending. Once a cycle, fewer loops (two at a time while more than four play), the
 * fullest row first and, of equal rows, the beat before the tunes, the tunes before the texture and the drones last;
 * nothing new comes in and the chords stay. When the last has gone, autoplay is off */
#define AUTO_END_PRI(f) ((f) == 3 ? 3 : (f) == 2 ? 2 : (f) == FAM_TEX ? 1 : 0)

static int auto_playing(void)
{
    int l, n = 0;
    for (l = 0; l < TP_NLOOP; l++)
        n += ui.want[l] != 0;
    return n;
}

static void auto_end_step(void)
{
    int l, f, k, n = auto_playing();
    for (k = n > 4 ? 2 : 1; k > 0 && n > 0; k--, n--) {
        int cnt[4] = {0, 0, 0, 0}, best = -1, score = -1;
        for (l = 0; l < TP_NLOOP; l++)
            if (ui.want[l])
                cnt[l / 4]++;
        for (f = 0; f < 4; f++)
            if (cnt[f] && cnt[f] * 4 + AUTO_END_PRI(f) > score) {
                score = cnt[f] * 4 + AUTO_END_PRI(f);
                best = f;
            }
        auto_set(auto_pick(best, 1), 0);
    }
    if (!auto_playing()) {
        ui.autoplay = 0;
        tp_hold(1);                          /* stopped: the clock back at bar 1 for the next start */
        app_say("AUTOPLAY", "THE END");
        ui.redraw |= 7;
    }
}

static void auto_on(int on)
{
    int l;
    ui.autoplay = (uint8_t)on;
    if (on == 2) {                           /* the ending: the first loop out at once */
        ui.auto_cycle = (uint32_t)(tp_step + 4) >> 6;
        ui.redraw |= 7;
        auto_end_step();
        return;
    }
    if (!on)
        return;
    ui.rnd ^= plat_ticks24();
    for (l = 0; l < TP_NLOOP; l++)
        ui.auto_age[l] = (uint16_t)rnd(3);
    tp_hold(0);                              /* the clock going, if it was held */
    ui.auto_n = 0;
    ui.auto_cycle = (uint32_t)(tp_step + 4) >> 6;
    auto_step();                             /* something at once */
}

/* ------------------------------------------------------- the recorder --- */
enum { R_IDLE, R_SETUP, R_COUNT, R_TAKE };

static void live_release(void)               /* every key's live note let go */
{
    uint32_t k;
    tp_live_all_off();
    for (k = 0; k < NKEYS; k++)
        ui.live[k] = 0;
}

static void slot_text(const char *a, int l, char *b)   /* "LOOP 5", after a */
{
    char n[4];
    b[0] = 0;
    text_cat(b, a);
    int_text(l + 1, n);
    text_cat(b, n);
}

static void bars_text(int bars, char *b)
{
    int_text(bars, b);
    text_cat(b, bars == 1 ? " BAR" : " BARS");
}

static const char *snap_text(int snap)
{
    return snap == TP_SNAP_16 ? "1/16" : snap == TP_SNAP_8 ? "1/8" : snap == TP_SNAP_BAR ? "BAR" : "OFF";
}

static void oct_text(char *b)                /* the keys' lowest note: F3 at 0 */
{
    char n[4];
    b[0] = 0;
    text_cat(b, "F");
    int_text(3 + set.oct, n);
    text_cat(b, n);
    text_cat(b, " UP");
}

static void rec_mode_set(int on)
{
    if (!on && (ui.rs == R_COUNT || ui.rs == R_TAKE)) {
        tp_rec_cancel();
        app_say("TAKE CANCELLED", "PLAYBACK");
    } else if (!on)
        app_say("PLAYBACK", "KEYS: THE LOOPS");
    else
        app_say("RECORDING MODE", "REC: A NEW LOOP");
    ui.rs = R_IDLE;
    if (on && ui.autoplay) {                 /* recording takes the loops back */
        auto_on(0);
        app_say("RECORDING MODE", "AUTOPLAY OFF");
    }
    live_release();
    touch_on(on);                            /* the keys' velocity, while they play */
    ui.rec_mode = (uint8_t)on;
    ui.redraw |= 7;
}

static int free_slot(void)
{
    int l;
    for (l = 0; l < TP_NLOOP; l++)
        if (!tp_user[l].used)
            return l;
    return -1;
}

static void rec_button(void)
{
    char t[24];
    int l, keep = 0;
    if (!ui.rec_mode) {
        app_say("REC", "GLO: RECORDING MODE");
        return;
    }
    switch (ui.rs) {
    case R_IDLE:                             /* a new loop, in the next free slot */
        if ((l = free_slot()) < 0) {
            app_say("ALL 16 LOOPS USED", "EDIT + KEY: DELETE");
            return;
        }
        ui.rslot = (uint8_t)l;
        ui.rs = R_SETUP;
        slot_text("NEW LOOP ", l, t);
        app_say(t, "REC AGAIN: COUNT-IN");
        break;
    case R_SETUP: {                          /* armed: a bar's count-in from the next bar line */
        tp_user_t *U = &tp_user[ui.rslot];
        U->instr = set.instr;
        U->bars = set.bars;
        U->snap = set.snap;
        tp_rec_arm(ui.rslot, set.click);
        tp_hold(0);                          /* a held clock starts: the count-in at once */
        ui.rs = R_COUNT;
        app_say("COUNT-IN", "FROM THE NEXT BAR");
        break;
    }
    case R_COUNT:                            /* before it starts: nothing to keep */
        tp_rec_cancel();
        ui.rs = R_IDLE;
        app_say("CANCELLED", "REC: A NEW LOOP");
        break;
    default:                                 /* the take kept, and it plays on (the first loop too: ready for a second) */
        keep = 1;
        tp_rec_finish(keep);
        ui.rkeep = (uint8_t)keep;
        ui.rsaving = 1;                      /* rec_poll saves it once the engine has closed it */
        ui.rs = R_IDLE;
        break;
    }
    ui.redraw |= 7;
}

/* each frame: the take starting, and a finished one saved (once the engine has closed its notes) */
static void rec_poll(void)
{
    tp_rec_info_t ri;
    char t[24];
    tp_rec_info(&ri);
    if (ui.rs == R_COUNT && ri.state == TP_REC_ON) {
        ui.rs = R_TAKE;
        app_say("RECORDING", "REC: DONE  GLO: CANCEL");
    }
    if (ui.rsaving && ri.state == TP_REC_OFF) {
        int l = ui.rslot;
        ui.rsaving = 0;
        slot_text("LOOP ", l, t);
        if (tp_user[l].used) {
            ui.want[l] = ui.rkeep;
            if (!ui.rkeep)
                ui.last_set |= (uint16_t)(1u << l);   /* PLAY brings it in */
            ui.last_slot = (int8_t)l;
            app_say(t, canvas_save(l) ? "SAVE FAILED" : ui.rkeep ? "SAVED, PLAYING" : "SAVED - PLAY: HEAR IT");
        } else
            app_say("NOTHING RECORDED", "");
        ui.redraw |= 7;
    }
}

/* EDIT held, a white key: pick the loop, the same key again deletes it, any other cancels */
static void del_key(int l)
{
    char t[24];
    if (ui.rs == R_COUNT || ui.rs == R_TAKE) {
        app_say("DELETE", "FINISH THE TAKE FIRST");
        return;
    }
    slot_text("LOOP ", l, t);
    if (ui.del_sel == l) {
        uint16_t bit = (uint16_t)(1u << l);
        set_loop(l, 0);                      /* what sounds rings out */
        tp_user[l].used = 0;
        tp_user[l].n = 0;
        ui.last_set &= (uint16_t)~bit;
        if (ui.last_slot == l)
            ui.last_slot = -1;
        ui.del_sel = -1;
        app_say(t, canvas_save(l) ? "DELETED, SAVE FAILED" : "DELETED");
    } else if (ui.del_sel >= 0) {
        ui.del_sel = -1;
        app_say("DELETE", "CANCELLED");
    } else if (!tp_user[l].used)
        app_say(t, "EMPTY");
    else {
        ui.del_sel = (int8_t)l;
        text_cat(t, "?");
        app_say(t, "TAP AGAIN TO DELETE");
    }
    ui.redraw |= 2;
}

/* recording mode: the keys play the instrument, live, at their velocity */
static void live_keys(uint32_t keys)
{
    uint32_t k;
    for (k = 0; k < NKEYS; k++) {
        uint32_t bit = 1u << k;
        if ((keys & bit) && !(ui.keys & bit)) {
            if (ui.live[k])
                tp_live_off(ui.live[k]);
            ui.live[k] = tp_live_on(set.instr, 53 + (int)k + (tp_instr_drums(set.instr) ? 0 : 12 * set.oct),
                                    touch_vel(k));   /* a drum kit: a sound a key, whatever the octave */
        } else if (!(keys & bit) && (ui.keys & bit) && ui.live[k]) {
            tp_live_off(ui.live[k]);
            ui.live[k] = 0;
        }
    }
}

/* recording mode: PRESET and KNOB1-3 set a new loop up (not during a take) */
static void rec_knob(uint32_t k, int32_t e)
{
    static const uint8_t BARS[3] = {1, 2, 4};
    char t[16];
    if (ui.rs == R_COUNT || ui.rs == R_TAKE) {
        app_say("AFTER THE TAKE", "REC: DONE");
        return;
    }
    if (k == EN_PRESET) {
        set.instr = (uint8_t)clamp((int32_t)set.instr + e, 0, TP_NINSTR - 1);
        live_release();
        instr_name(set.instr, t);
        app_say("INSTRUMENT", t);
    } else if (k == EN_K1) {
        int i = set.bars == 1 ? 0 : set.bars == 2 ? 1 : 2;
        set.bars = BARS[clamp(i + (e > 0 ? 1 : -1), 0, 2)];
        bars_text(set.bars, t);
        app_say("LENGTH", t);
    } else if (k == EN_K2) {
        set.snap = (uint8_t)clamp((int32_t)set.snap + (e > 0 ? 1 : -1), 0, TP_NSNAP - 1);
        app_say("SNAP", snap_text(set.snap));
    } else if (k == EN_K3) {
        set.click = e > 0;
        app_say("CLICK", set.click ? "ON" : "OFF");
    } else {
        app_say("KNOB 4", "NOT USED HERE");
        return;
    }
    set_dirty = 1;
    ui.redraw |= 6;
}

static void input(void)
{
    uint32_t keys, btn = plat_buttons(), k;
    int32_t e;
    int rec = IS_CANVAS && ui.rec_mode;
    if (touch_is_on()) {                     /* recording mode: the keys and their velocity from lib/touch */
        touch_poll();
        keys = touch_keys();
    } else
        keys = plat_keys();
    if ((keys & ~ui.keys) || (btn & ~ui.btn))
        ui.touched = 1;
    if (rec && !ui.edit_down)                /* recording: the keys play */
        live_keys(keys);
    else {
        for (k = 0; k < 16u; k++) {          /* white keys: the loops (not while a bank switch fades out) */
            uint32_t bit = 1u << WK[k];
            if ((keys & bit) && !(ui.keys & bit)) {
                if (ui.switching)
                    app_say("CHANGING BANK", "");
                else if (ui.edit_down)
                    del_key(loop_at((int)k));
                else
                    toggle(loop_at((int)k));
            }
        }
        for (k = 0; k < 11u; k++) {          /* black keys: not yet (with EDIT: a delete cancelled) */
            uint32_t bit = 1u << BK[k];
            if ((keys & bit) && !(ui.keys & bit)) {
                if (ui.edit_down && ui.del_sel >= 0) {
                    ui.del_sel = -1;
                    app_say("DELETE", "CANCELLED");
                } else
                    app_say(BLACK_NAME[k], "NOT USED YET");
            }
        }
    }
    for (k = 0; k < NB; k++) {
        uint32_t bit = 1u << k;
        if (!(btn & bit) || (ui.btn & bit))
            continue;
        if ((k == B_OCTDN || k == B_OCTUP) && rec) {   /* recording: the keys' octave */
            char t[12];
            set.oct = (int8_t)clamp(set.oct + (k == B_OCTUP ? 1 : -1), -2, 2);
            set_dirty = 1;
            live_release();
            oct_text(t);
            app_say("OCTAVE", tp_instr_drums(set.instr) ? "DRUMS: NONE" : t);
            ui.redraw |= 2;
        } else if (k == B_OCTDN || k == B_OCTUP) {
            ui.setting = (uint8_t)((ui.setting + (k == B_OCTUP ? 1 : nset() - 1)) % nset());
            app_say("KNOBS", SET_NAME[ui.setting]);
            ui.redraw |= 6;
        } else if ((k == B_PLAY || k == B_HOME || k == B_GLO) && ui.switching)
            app_say("CHANGING BANK", "");
        else if (k == B_PLAY) {              /* acted on at its release (below): held, KNOB1 picks the chords */
            ui.play_down = 1;
            ui.play_turned = 0;
            ui.play_t = stamp();
        }
        else if (k == B_SAVE) {
            int l, bad = plat_store_save(OBJ_SETTINGS, &set, sizeof set) != 0;
            for (l = 0; l < TP_NLOOP; l++)   /* and the canvas's loops whose instrument changed */
                if (cv_dirty >> l & 1u)
                    bad |= canvas_save(l) != 0;
            if (!bad) {
                set_dirty = 0;
                app_say("SAVED", "BANKS AND TEMPO");
            } else
                app_say("SAVE", "FAILED");
        } else if (k == B_HOME && rec)      /* recording mode: HOME leaves it, as GLO does */
            rec_mode_set(0);
        else if (k == B_HOME && IS_CANVAS)
            app_say("SURPRISE", "NOT ON A CANVAS");
        else if (k == B_HOME) {              /* held 3 s: a surprise (home_hold counts it down) */
            ui.home_t = stamp();
            ui.home_fired = 0;
            ui.home_left = 0;
        } else if (k == B_GLO && IS_CANVAS)
            rec_mode_set(!ui.rec_mode);
        else if (k == B_REC && IS_CANVAS)
            rec_button();
        else if (k == B_EDIT && IS_CANVAS) { /* held: the loops, a key to delete one */
            ui.edit_down = 1;
            ui.del_sel = -1;
            if (rec)
                live_release();
            app_say("DELETE A LOOP", "TAP ITS KEY");
            ui.redraw |= 2;
        } else if (k == B_SEQ && ui.autoplay == 1) {
            app_say("AUTOPLAY", "ENDING: SEQ TO GO ON");
            auto_on(2);
        } else if (k == B_SEQ && rec && (ui.rs == R_COUNT || ui.rs == R_TAKE))
            app_say("AUTOPLAY", "FINISH THE TAKE FIRST");
        else if (k == B_SEQ) {
            int l, any = 0;
            if (rec)                         /* from recording mode: back to playback first */
                rec_mode_set(0);
            for (l = 0; l < TP_NLOOP; l++)
                any |= auto_avail(l);
            if (!any)
                app_say("AUTOPLAY", "NOTHING RECORDED YET");
            else {
                auto_on(1);
                app_say("AUTOPLAY", "SEQ AGAIN: END");
            }
            ui.redraw |= 7;
        } else if ((k == B_GLO || k == B_REC) && !IS_CANVAS)
            app_say(BTN_NAME[k], "BLANK CANVAS ONLY");
        else
            app_say(BTN_NAME[k], "NOT USED YET");
    }
    if (ui.edit_down && !(btn >> B_EDIT & 1u)) {   /* EDIT let go: no delete waiting */
        ui.edit_down = 0;
        ui.del_sel = -1;
        ui.redraw |= 2;
    }
    home_hold(btn);
    if (ui.play_down && !(btn >> B_PLAY & 1u)) {   /* PLAY let go: stop or play, unless it picked chords */
        ui.play_down = 0;
        if (rec && (ui.rs == R_COUNT || ui.rs == R_TAKE))
            app_say("PLAY", "REC: FINISH THE TAKE");
        else if (ui.play_hint)               /* held long enough to offer the chords: a look, not a stop */
            ;
        else if (!ui.play_turned && !ui.switching) {
            if (ui.autoplay)                 /* PLAY takes over from autoplay */
                auto_on(0);
            play_button();
        }
    }
    for (k = 0; k < NE; k++) {
        char t[16];
        if ((e = plat_enc((int)k)) == 0)
            continue;
        ui.touched = 1;
        if (k == EN_SELECT) {
            set.bpm = (uint8_t)clamp((int32_t)set.bpm + e, 56, 140);
            tp_bpm((float)set.bpm);
            set_dirty = 1;
            int_text(set.bpm, t);
            text_cat(t, " BPM");
            app_say("TEMPO", t);
        } else if (k == EN_ALGO) {
            ui.row = (uint8_t)clamp((int32_t)ui.row + e, 0, 3);
            fam_name(ui.row, t);
            app_say("KNOBS", t);
            ui.redraw |= 2;
        } else if (rec && (k == EN_PRESET || k >= EN_K1))
            rec_knob(k, e);
        else if (k == EN_PRESET) {
            char b[16];
            ui.pick = (uint8_t)clamp((int32_t)ui.pick + e, 0, NBK - 1);
            ui.pick_at = plat_ms();          /* it changes once the knob rests (app_frame) */
            t[0] = 0;
            text_cat(t, "BANK ");
            int_text(ui.pick + 1, b);
            text_cat(t, b);
            bank_name(ui.pick, b);
            app_say(t, b);
        } else if (k == EN_K1 && ui.play_down && !IS_CANVAS) {   /* PLAY + KNOB1: the chord progression */
            ui.play_turned = 1;
            chord_knob(e);
        } else
            knob((int)(k - EN_K1), e);
    }
    ui.keys = keys;
    ui.btn = btn;
}

void app_input_only(void) { input(); }

static void ui_reset(void)
{
    memset(&ui, 0, sizeof ui);
    ui.last_slot = -1;
    ui.del_sel = -1;
}

static uint16_t mix565(uint16_t a, uint16_t b, int32_t t)   /* a .. b, t 0..256 */
{
    int32_t ra = a >> 11, ga = (a >> 5) & 63, ba = a & 31, rb = b >> 11, gb = (b >> 5) & 63, bb = b & 31;
    return (uint16_t)(((ra + ((rb - ra) * t >> 8)) << 11) | ((ga + ((gb - ga) * t >> 8)) << 5) |
                      (ba + ((bb - ba) * t >> 8)));
}

/* the header: the bank and the tempo, or a message for a moment; the chord cycle under it */
static void draw_header(uint32_t now)
{
    const theme_t *T = &THEMES[set.bank];
    uint16_t hi = theme_ink(T, 1), lo = theme_ink(T, 0);
    char t[20];
    int32_t i, step = tp_step, bar = (step % 64) / 16, within = step % 16;
    cv_begin(240, 46, T->bg);
    if (now < ui.msg_until) {                /* a message: on the bank's colour, so it reads over any banner */
        int32_t x;
        cv_rect(0, 0, 240, 22, T->bg);
        x = cv_text(8, 3, &FONT_B, ui.msg[0], hi);
        cv_text(x + 8, 3, &FONT_B, ui.msg[1], T->accent);
    } else {                                  /* the bank's scene, its name over it in pixel letters */
        banner(set.bank, now);
        if (IS_CANVAS && ui.rec_mode) {      /* recording mode: so it says, a red light while a take runs */
            int32_t x = px_text(6, 4, 2, "RECORDING", T->title, T->title_shadow, 1);
            if (ui.rs == R_TAKE ? 1 : ui.rs == R_COUNT ? (int)((now / 250u) & 1u) : 0) {
                cv_rect(x + 6, 6, 10, 10, RGB(255, 252, 244));
                cv_rect(x + 7, 7, 8, 8, RGB(214, 48, 40));
            }
        } else {
            bank_name(set.bank, t);
            px_text(6, 4, 2, t, T->title, T->title_shadow, 1);
        }
        int_text(set.bpm, t);
        text_cat(t, " BPM");
        cv_text(233 - text_w(&FONT_B, t), 4, &FONT_B, t, T->title_shadow);
        cv_text(232 - text_w(&FONT_B, t), 3, &FONT_B, t, T->accent);
    }
    if (ui.home_t && !ui.home_fired) {       /* HOME held: the countdown fills a bar */
        uint32_t held = now - ui.home_t;
        cv_rect(8, 22, 224, 2, T->cell_sel);
        cv_rect(8, 22, (int32_t)(224u * (held > 3000u ? 3000u : held) / 3000u), 2, hi);
    }
    for (i = 0; i < 4; i++) {                /* the chord cycle: the bar playing fills as it goes */
        int32_t x = 8 + i * 57;
        cv_rect(x, 26, 53, 16, T->cell);
        if (ui.play_hint) {                  /* PLAY held: KNOB1 turns these */
            cv_rect(x - 1, 25, 55, 1, T->accent);
            cv_rect(x - 1, 42, 55, 1, T->accent);
            cv_rect(x - 1, 25, 1, 18, T->accent);
            cv_rect(x + 53, 25, 1, 18, T->accent);
        }
        if (i == bar)
            cv_rect(x, 26, 53 * (within + 1) / 16, 16, T->chord);
        cv_text(x + 4, 27, &FONT_T, tp_chords()[i].name, i == bar ? C_WHITE : lo);
    }
    cv_blit(0, 0);
    lcd_fill(0, 46, 240, 2, T->bg);           /* the seam between the header and the pools */
}

/* two rows of loops (a strip of 80 lines): each cell glows with its loop's level; the knobs' row shows their values */
static void draw_rows(int32_t r0, uint32_t now)
{
    const theme_t *T = &THEMES[set.bank];
    int32_t r, c;
    cv_begin(240, 80, T->bg);
    for (r = r0; r < r0 + 2; r++)
        for (c = 0; c < 4; c++) {
            int l = loop_at(r * 4 + c), st = tp_loop_state(l), sel = r == ui.row;
            int32_t x = 8 + c * 57, y = (r - r0) * 40 + 2;
            uint16_t bg = sel ? T->cell_sel : T->cell, fam = T->fam[row_fam(r)];
            uint16_t ink = T->light ? theme_ink(T, sel) : sel ? RGB(201, 203, 207) : C_GRAY;
            char n[20], *w2 = 0, *p;
            if (IS_CANVAS && !tp_user[l].used) {   /* an empty loop: a pencilled outline and its number */
                uint16_t line = mix565(T->bg, ink, 90);
                int32_t k;
                cv_rect(x, y, 53, 34, T->bg);
                for (k = 0; k < 53; k += 4) {
                    cv_rect(x + k, y, 2, 1, line);
                    cv_rect(x + k, y + 33, 2, 1, line);
                }
                for (k = 0; k < 34; k += 4) {
                    cv_rect(x, y + k, 1, 2, line);
                    cv_rect(x + 52, y + k, 1, 2, line);
                }
                int_text(l + 1, n);
                cv_text(x + 27 - text_w(&FONT_T, n) / 2, y + 11, &FONT_T, n, line);
                continue;
            }
            if (st & 4) {                    /* sounding: the family's colour, brighter with the level */
                float lv = fm_sqrtf(tp_loop_level(l)) * 6.0f;
                int32_t t = 102 + (int32_t)((lv > 1.0f ? 1.0f : lv) * 154.0f);   /* 40 % .. the full colour */
                if (T->shape == SHAPE_NEON)
                    t = t * 5 / 8;               /* neon: a darker glass inside a bright tube */
                if (T->light)
                    t = 150 + (t - 102) * 106 / 154;   /* paper: a strong wash, for the white */
                bg = mix565(bg, fam, t);
                ink = C_WHITE;
            }
            cv_rect(x, y, 53, 34, bg);
            if (T->shape == SHAPE_ZEN && !(st & 4) && !sel) {   /* raked gravel, while a pool is still */
                int32_t k;
                for (k = 2; k < 51; k++) {
                    cv_rect(x + k, y + 26 + (int32_t)((k / 7) & 1), 1, 1, mix565(bg, C_WHITE, 26));
                    cv_rect(x + k, y + 30 + (int32_t)((k / 7) & 1), 1, 1, mix565(bg, C_WHITE, 18));
                }
            }
            if (T->shape == SHAPE_ARCH && !(st & 4)) {   /* the dungeon's pools are stone: mortar courses */
                cv_rect(x, y + 11, 53, 1, mix565(bg, C_BLACK, 70));
                cv_rect(x, y + 22, 53, 1, mix565(bg, C_BLACK, 70));
            }
            if (T->shape == SHAPE_ROUND && (st & 4)) {   /* Shallows: a waterline moving along the top */
                int32_t k;
                for (k = 0; k < 53; k++)
                    cv_rect(x + k, y + 1 + (int32_t)(((uint32_t)k / 4u + now / 160u) % 3u == 0u), 1, 1,
                            mix565(bg, C_WHITE, 120));
            }
            if (T->shape == SHAPE_NEON) {        /* the tube: bright when playing, a faint line when not */
                uint16_t tube = (st & 4) ? fam : mix565(bg, fam, 70);
                cv_rect(x, y, 53, 1, tube);
                cv_rect(x, y + 33, 53, 1, tube);
                cv_rect(x, y, 1, 34, tube);
                cv_rect(x + 52, y, 1, 34, tube);
            }
            if ((st & 2) && (now / 250u) & 1u) {   /* joining on the beat: a blinking frame */
                cv_rect(x, y, 53, 1, fam);
                cv_rect(x, y + 33, 53, 1, fam);
                cv_rect(x, y, 1, 34, fam);
                cv_rect(x + 52, y, 1, 34, fam);
            }
            if (ui.edit_down && ui.del_sel == l && (now / 150u) & 1u) {   /* picked to delete: a red frame */
                uint16_t red = RGB(214, 48, 40);
                cv_rect(x, y, 53, 2, red);
                cv_rect(x, y + 32, 53, 2, red);
                cv_rect(x, y, 2, 34, red);
                cv_rect(x + 51, y, 2, 34, red);
            }
            pool_shape(T->shape, x, y, 53, 34, T->bg, l);
            n[0] = 0;
            text_cat(n, IS_CANVAS ? tp_instr_name(tp_user[l].instr) : BANK->loops[l].name);
            for (p = n; *p; p++)             /* two words, two lines */
                if (*p == ' ') {
                    *p = 0;
                    w2 = p + 1;
                    break;
                }
            if (!w2 && text_w(&FONT_T, n) > 47) {   /* one long word: hyphenated over two lines */
                static char tail[20];
                uint32_t h = 7, j;
                for (j = 0; n[h + j]; j++)
                    tail[j] = n[h + j];
                tail[j] = 0;
                n[h] = '-';
                n[h + 1] = 0;
                w2 = tail;
            }
            if (st & 4) {                    /* white on a bright colour: a dark shadow under it keeps it readable */
                uint16_t sh = mix565(bg, C_BLACK, 150);
                cv_text(x + 5, y + 3, &FONT_T, n, sh);
                if (w2)
                    cv_text(x + 5, y + 14, &FONT_T, w2, sh);
            }
            cv_text(x + 4, y + 2, &FONT_T, n, ink);
            if (w2)
                cv_text(x + 4, y + 13, &FONT_T, w2, ink);
            if (sel) {                       /* the knobs' row: each loop's value of the setting */
                int32_t u = ui.setting == S_INSTR ? tp_user[l].instr * 100 / (TP_NINSTR - 1) : USET[l][ui.setting];
                uint16_t on = (st & 4) ? C_WHITE : fam;
                cv_rect(x + 4, y + 27, 45, 3, (st & 4) ? mix565(bg, C_BLACK, 90) : mix565(bg, C_WHITE, 30));
                if (ui.setting == S_PAN) {
                    cv_rect(x + 26, y + 25, 1, 7, on);
                    cv_rect(x + 4 + 43 * u / 100, y + 25, 3, 7, on);
                } else
                    cv_rect(x + 4, y + 27, 45 * u / 100, 3, on);
            }
        }
    for (r = r0; r < r0 + 2; r++)            /* the knobs' row: a bar at its left */
        if (r == ui.row)
            cv_rect(2, (r - r0) * 40 + 2, 3, 34, T->fam[row_fam(r)]);
    cv_blit(0, (uint32_t)(48 + r0 * 40));     /* rows 0-1 at 48, rows 2-3 at 128 */
}

/* recording mode, the middle of the screen (two strips of 80 lines): a new loop's instrument, length, snap, click and
 * the keys' octave; then its bars with the notes going in and the playhead (or the loop recorded last) */
static void draw_recorder(uint32_t now)
{
    static const char *const LABEL[4] = {"LENGTH", "SNAP", "CLICK", "KEYS"}, *const KNOB[4] = {"K1", "K2", "K3", "OCT"};
    const theme_t *T = &THEMES[set.bank];
    uint16_t hi = theme_ink(T, 1), lo = theme_ink(T, 0);
    int busy = ui.rs == R_COUNT || ui.rs == R_TAKE, s, bars, len, i;
    char t[24];
    tp_rec_info_t ri;
    tp_rec_info(&ri);
    cv_begin(240, 80, T->bg);
    cv_text(8, 2, &FONT_T, "INSTRUMENT", lo);
    cv_text(232 - text_w(&FONT_T, "PRESET"), 2, &FONT_T, "PRESET", lo);
    instr_name(set.instr, t);
    cv_text(8, 13, &FONT_B, t, busy ? lo : T->accent);
    for (i = 0; i < 4; i++) {
        int32_t x = 8 + i * 57;
        cv_rect(x, 34, 53, 32, T->cell);
        cv_text(x + 4, 36, &FONT_T, LABEL[i], lo);
        cv_text(x + 49 - text_w(&FONT_T, KNOB[i]), 36, &FONT_T, KNOB[i], lo);
        if (i == 0)
            bars_text(set.bars, t);
        else if (i == 1) {
            t[0] = 0;
            text_cat(t, snap_text(set.snap));
        } else if (i == 2) {
            t[0] = 0;
            text_cat(t, set.click ? "ON" : "OFF");
        } else if (tp_instr_drums(set.instr)) {   /* a drum kit: no octave */
            t[0] = 0;
            text_cat(t, "KIT");
        } else {
            t[0] = 0;
            text_cat(t, "F");
            int_text(3 + set.oct, t + 1);
        }
        cv_text(x + 4, 48, &FONT_B, t, busy && i < 3 ? lo : hi);
    }
    cv_blit(0, 48);

    s = ui.rs != R_IDLE || ui.rsaving ? ui.rslot : ui.last_slot;   /* the loop shown */
    bars = s >= 0 && (busy || ui.rs == R_IDLE) && tp_user[s].bars ? tp_user[s].bars : set.bars;
    len = bars * 16;
    cv_begin(240, 80, T->bg);
    {
        const int32_t x0 = 8, y0 = 2, w = 224, h = 60;
        int32_t lo_n = 127, hi_n = 0, span, n = s >= 0 ? tp_user[s].n : 0, play = -1;
        uint16_t ncol = s >= 0 ? T->fam[s / 4] : T->accent;
        if (s >= 0 && !tp_user[s].used && !busy)
            n = 0;
        cv_rect(x0, y0, w, h, T->cell);
        for (i = 0; i <= bars * 4; i++) {    /* the beats, the bars bolder */
            int32_t x = x0 + i * (w - 1) / (bars * 4);
            cv_rect(x, y0, i % 4 ? 1 : 2, h, i % 4 ? mix565(T->cell, T->cell_sel, 160) : mix565(T->cell_sel, lo, 90));
        }
        for (i = 0; i < bars; i++) {         /* their numbers under them */
            int_text(i + 1, t);
            cv_text(x0 + 2 + i * w / bars, y0 + h + 2, &FONT_T, t, lo);
        }
        for (i = 0; i < n; i++) {            /* the notes' range: an octave at least */
            int32_t m = tp_user[s].notes[i].note;
            lo_n = m < lo_n ? m : lo_n;
            hi_n = m > hi_n ? m : hi_n;
        }
        span = n ? hi_n - lo_n : 0;
        if (span < 12) {
            lo_n -= (12 - span) / 2;
            span = 12;
        }
        for (i = 0; i < n; i++) {            /* each note: where it plays (snapped as it plays), how long */
            const tp_note_t *N = &tp_user[s].notes[i];
            int32_t p, d, px, pw, py;
            tp_user_place(&tp_user[s], N, &p, &d);
            px = x0 + p * w / (len * 256);
            pw = d * w / (len * 256);
            pw = pw < 4 ? 4 : pw;
            py = y0 + h - 7 - (N->note - lo_n) * (h - 11) / span;
            if (px + pw > x0 + w) {          /* over the end: round to the start */
                cv_rect(x0, py, px + pw - x0 - w, 4, ncol);
                pw = x0 + w - px;
            }
            cv_rect(px, py, pw, 4, ncol);
        }
        if (ri.state == TP_REC_ON)          /* the playhead: the take's, or the loop's as it plays */
            play = (int32_t)ri.pos;
        else if (s >= 0 && ui.rs == R_IDLE && (tp_loop_state(s) & 1))
            play = tp_step % len;
        if (play >= 0)
            cv_rect(x0 + play * w / len, y0, 2, h, ui.rs == R_TAKE ? RGB(214, 48, 40) : hi);
        if (ri.state == TP_REC_COUNTIN && ri.beats_to_go <= 4) {   /* the count-in: the beats to go, big */
            int_text(ri.beats_to_go, t);
            px_text(120 - px_text_w(t, 5) / 2, y0 + 12, 5, t, RGB(214, 48, 40), T->title_shadow, 1);
        } else if (ri.state == TP_REC_COUNTIN)   /* waiting for the bar line the count-in starts on */
            cv_text(x0 + (w - text_w(&FONT_B, "NEXT BAR")) / 2, y0 + 22, &FONT_B, "NEXT BAR", lo);
        else if (s < 0)
            cv_text(x0 + (w - text_w(&FONT_B, "REC: A NEW LOOP")) / 2, y0 + 22, &FONT_B, "REC: A NEW LOOP", lo);
        else if (ui.rs == R_SETUP) {
            slot_text("LOOP ", s, t);
            text_cat(t, " - REC: GO");
            cv_text(x0 + (w - text_w(&FONT_B, t)) / 2, y0 + 22, &FONT_B, t, lo);
        }
    }
    (void)now;
    cv_blit(0, 128);
}

/* the footer: what the knobs change, and the FM-1's own audio load */
static void draw_footer(void)
{
    char t[24];
    const theme_t *T = &THEMES[set.bank];
    uint16_t hi = theme_ink(T, 1), lo = theme_ink(T, 0);
    uint32_t cpu = plat_cpu_pct(), xr = plat_xruns(), i;
    cv_begin(240, 32, T->bg);
    if (IS_CANVAS && ui.rec_mode && !ui.edit_down) {   /* recording: what REC does next */
        static const char *const NEXT[4] = {"REC: NEW LOOP", "REC: COUNT-IN", "REC: CANCEL", "REC: DONE"};
        cv_text(8, 4, &FONT_B, NEXT[ui.rs], T->accent);
        cv_text(8, 20, &FONT_T, ui.rs >= R_COUNT ? "GLO/HOME: CANCEL TAKE" : "GLO/HOME: PLAYBACK", lo);
    } else if (ui.edit_down) {
        cv_text(8, 4, &FONT_B, "DELETE", RGB(214, 48, 40));
        cv_text(8, 20, &FONT_T, "KEY, THEN SAME KEY", lo);
    } else {
        t[0] = 0;
        text_cat(t, "< ");
        text_cat(t, SET_NAME[ui.setting]);
        text_cat(t, " >");
        cv_text(8, 4, &FONT_B, t, T->fam[row_fam(ui.row)]);
        for (i = 0; i < (uint32_t)nset(); i++)
            cv_rect(9 + (int32_t)i * 7, 24, 5, 3, i == ui.setting ? hi : T->cell_sel);
        if (ui.autoplay)
            cv_text(68, 20, &FONT_T, ui.autoplay == 2 ? "ENDING" : "AUTOPLAY", T->accent);
    }
    int_text((int32_t)cpu, t);
    text_cat(t, "%");
    cv_text(150, 8, &FONT_T, "CPU", lo);
    cv_text(172, 6, &FONT_B, t, cpu > 85u ? RGB(224, 138, 82) : hi);
    cv_rect(206, 10, 28, 7, T->cell);
    cv_rect(206, 10, 28 * (int32_t)(cpu > 100u ? 100u : cpu) / 100, 7, cpu > 85u ? RGB(224, 138, 82) : T->accent);
    if (xr) {                                /* audio dropouts since boot: the render missed its time */
        int_text((int32_t)xr, t);
        text_cat(t, " XRUN");
        cv_text(150, 20, &FONT_T, t, RGB(224, 138, 82));
    }
    if (tp_stats.shed) {                     /* notes faded early to keep out of an overload (app_overload) */
        int_text((int32_t)tp_stats.shed, t);
        text_cat(t, " SHED");
        cv_text(196, 20, &FONT_T, t, RGB(224, 171, 79));
    }
    cv_blit(0, 208);
}

/* a screen reader that has just started listening (or asks) hears the whole screen */
static void speak(void)
{
    char t[96], b[20];
    if (ann_on() && ann_where()) {
        t[0] = 0;
        text_cat(t, APP_NAME ". ");
        bank_name(set.bank, b);
        text_cat(t, b);
        text_cat(t, ". ");
        int_text(set.bpm, b);
        text_cat(t, b);
        text_cat(t, " BPM. KNOBS ");
        text_cat(t, SET_NAME[ui.setting]);
        text_cat(t, ", ");
        fam_name(ui.row, b);
        text_cat(t, b);
        ann_say(t, 0, ANN_NOW);
    }
    ann_tick(plat_ms());
}

/* the first screen at power-on (fm1-base APP_SPLASH): the splash's first frame */
void app_splash(void) { splash_draw(0); }

void app_ui_init(void)
{
    ui.rnd = plat_ticks24() | 1u;
    ui.intro_t = stamp();                    /* the splash moves a moment longer: any key, button or knob ends it */
    ui.touched = 0;
    splash_draw(0);
}

void app_frame(void)
{
    static uint32_t last_grid, last_foot, last_step = 0xFFFFFFFFu, last_msg;
    uint32_t now = plat_ms(), keys = 0, btn = 0, l, playing = 0;
    input();
    rec_poll();
    play_hint(now);
    speak();
    if (ui.intro_t) {                        /* the splash: the sea moves for 2.6 s, or until something is touched */
        static uint32_t last_splash;
        if (!ui.touched && now - ui.intro_t < 2600u) {
            if (now - last_splash >= 80u) {
                last_splash = now;
                splash_draw(now - ui.intro_t);
            }
            plat_glow(1);
            plat_leds(0, 0);
            return;
        }
        ui.intro_t = 0;
        lcd_fill(0, 0, 240, 240, THEMES[set.bank].bg);
        ui.redraw = 7;
    }
    if (!ui.switching && ui.pick != set.bank && now - ui.pick_at >= 700u) {
        for (l = 0; l < TP_NLOOP; l++)       /* PRESET rested on another bank: everything out, quickly */
            ui.want[l] = 0;
        ui.last_set = 0;
        tp_stop_fast();
        ui.switching = 1;
    }
    if (ui.switching && tp_quiet()) {        /* quiet: the new bank, with its own settings, from its first chord */
        char b[20];
        set.bank = ui.pick;
        tp_set_bank(BANK);
        if (ui.setting >= nset())             /* INSTRUMENT is the canvas's */
            ui.setting = 0;
        apply_progression(1);                /* its own progression, at once: nothing plays yet */
        ui.prog_waiting = 0;
        for (l = 0; l < TP_NLOOP; l++)
            apply_loop((int)l);
        set_dirty = 1;
        ui.switching = 0;
        bank_name(set.bank, b);
        if (!ui.autoplay)                    /* held at bar 1 until PLAY or SEQ: loops can be picked first */
            tp_hold(1);
        app_say(b, ui.autoplay ? "AUTOPLAY" : "KEYS, THEN PLAY");
        if (ui.autoplay == 1) {              /* straight on in the new bank (an ending: nothing new) */
            ui.auto_cycle = (uint32_t)(tp_step + 4) >> 6;
            auto_step();
        }
        ui.redraw |= 7;
    }
    if (ui.prog_waiting && !ui.switching) {   /* a progression chosen: from the next bar line (at once if silent) */
        int quiet = 1;
        for (l = 0; l < TP_NLOOP; l++)
            if (tp_loop_state((int)l))
                quiet = 0;
        if (apply_progression(quiet))
            ui.prog_waiting = 0;
        ui.redraw |= 1;
    }
    if (ui.autoplay && !ui.switching) {      /* autoplay: once a cycle, a beat before its downbeat */
        uint32_t c = (uint32_t)(tp_step + 4) >> 6;
        if (c != ui.auto_cycle) {
            ui.auto_cycle = c;
            if (ui.autoplay == 2)
                auto_end_step();
            else
                auto_step();
        }
    }
    for (l = 0; l < TP_NLOOP; l++) {         /* the lights: playing lit, joining blinking; PLAY green, SAVE if changed */
        int st = tp_loop_state((int)l);
        if (st & 1)
            playing = 1;
        if ((st & 1) || ((st & 2) && (now / 250u) & 1u))
            keys |= 1u << WK[pos_of((int)l)];
    }
    if (IS_CANVAS && ui.edit_down) {         /* deleting: the loops recorded, the one picked blinking */
        keys = 0;
        for (l = 0; l < TP_NLOOP; l++)
            if (tp_user[l].used && ((int)l != ui.del_sel || (now / 150u) & 1u))
                keys |= 1u << WK[pos_of((int)l)];
    } else if (IS_CANVAS && ui.rec_mode)      /* recording: the keys held */
        keys = ui.keys;
    if (playing)
        btn |= 1u << B_PLAY;
    else if (tp_held() && !ui.switching)     /* loops picked, waiting: PLAY blinks to start them */
        for (l = 0; l < TP_NLOOP; l++)
            if (ui.want[l] && (now / 300u) & 1u)
                btn |= 1u << B_PLAY;
    if (set_dirty)
        btn |= 1u << B_SAVE;
    if (IS_CANVAS && ui.rec_mode) {          /* GLO lit; REC lit set up and recording, blinking for the count-in */
        btn |= 1u << B_GLO;
        if (ui.rs == R_SETUP || ui.rs == R_TAKE || (ui.rs == R_COUNT && (now / 250u) & 1u))
            btn |= 1u << B_REC;
    }
    if (ui.edit_down)
        btn |= 1u << B_EDIT;
    if (ui.autoplay == 1 || (ui.autoplay == 2 && (now / 400u) & 1u))   /* lit; blinking while it ends */
        btn |= 1u << B_SEQ;
    if (ui.home_t && !ui.home_fired) {       /* HOME held: lit, and the countdown bar moving */
        btn |= 1u << B_HOME;
        ui.redraw |= 1;
    }
    plat_glow(1);
    plat_leds(btn, keys);
    {
        static uint32_t last_banner;            /* the banner's scene moves: about 10 frames a second */
        if (now - last_banner >= 100u) {
            last_banner = now;
            ui.redraw |= 1;
        }
    }
    if ((uint32_t)tp_step != last_step) {    /* the chord cycle moves on each step */
        last_step = (uint32_t)tp_step;
        ui.redraw |= 1;
    }
    if (ui.msg_until && now >= ui.msg_until) {
        ui.msg_until = 0;
        ui.redraw |= 1;
    }
    if (ui.msg_until != last_msg) {
        last_msg = ui.msg_until;
        ui.redraw |= 1;
    }
    if (now - last_grid >= 66u) {            /* the levels: about 15 times a second */
        last_grid = now;
        ui.redraw |= 2;
    }
    if (now - last_foot >= 500u) {           /* the CPU load: twice a second */
        last_foot = now;
        ui.redraw |= 4;
    }
    if (ui.redraw & 1)
        draw_header(now);
    if ((ui.redraw & 2) && IS_CANVAS && ui.rec_mode && !ui.edit_down)
        draw_recorder(now);
    else if (ui.redraw & 2) {
        draw_rows(0, now);
        draw_rows(2, now);
    }
    if (ui.redraw & 4)
        draw_footer();
    ui.redraw = 0;
}

/* ------------------------------------------------------- SysEx bench --- */
/* The Mac's bench (tools/fm1_bench.py) over USB-MIDI, under the non-commercial manufacturer ID: F0 7D 54 cmd .. F7
 *   01        the measurements since the last 01 (reply F0 7D 54 41, then 3-byte values, 7 bits each, low first:
 *             render average and worst block in 1/1000 of their time, the stages' average (voices, mix, effects,
 *             compressor) in 1/1000, plat_cpu_pct, dropouts, voices now and their peak, the bank, the loops playing)
 *   02 m m m  the loops playing: bit l of the 21-bit mask (16 used); the clock runs while any is on
 *   03 b      bank b, now
 *   04        the voice peak and the worst block start again
 *   05 k u    a stress test: setting k (0 volume .. 5 pan) at knob position u on every loop, not saved
 *   06        the stress test over: every loop's saved settings again */
static void put21(uint8_t *f, uint32_t v)
{
    f[0] = (uint8_t)(v & 0x7Fu);
    f[1] = (uint8_t)(v >> 7 & 0x7Fu);
    f[2] = (uint8_t)(v >> 14 & 0x7Fu);
}

int app_sysex(const uint8_t *p, uint32_t n)
{
    uint8_t f[64];
    uint32_t k = 0, l;
    if (n < 3u || p[0] != 0x7Du || p[1] != 0x54u)
        return 0;
    if (p[2] == 0x01u) {
        uint32_t sum = pr_sum, mx = pr_max, bud = pr_budget / 1000u, prof[TP_PROF_N], mask = 0;   /* 32-bit only:
                                                     * felucca-bsp docs/CPU.md (no 64-bit multiplies beside loads) */
        for (l = 0; l < TP_PROF_N; l++) {
            prof[l] = tp_prof[l];
            tp_prof[l] = 0;
        }
        pr_sum = pr_budget = pr_blocks = 0;
        f[k++] = 0xF0u;
        f[k++] = 0x7Du;
        f[k++] = 0x54u;
        f[k++] = 0x41u;
        put21(f + k, bud ? sum / bud : 0u); k += 3;
        put21(f + k, mx / (APP_BLOCK * 544u / 1000u)); k += 3;
        for (l = 0; l < TP_PROF_N; l++) {
            put21(f + k, bud ? prof[l] / bud : 0u);
            k += 3;
        }
        put21(f + k, plat_cpu_pct()); k += 3;
        put21(f + k, plat_xruns()); k += 3;
        put21(f + k, (uint32_t)tp_stats.sounding); k += 3;
        put21(f + k, (uint32_t)tp_stats.peak); k += 3;
        put21(f + k, set.bank); k += 3;
        for (l = 0; l < TP_NLOOP; l++)
            if (tp_loop_state((int)l) & 1)
                mask |= 1u << l;
        put21(f + k, mask); k += 3;
        put21(f + k, tp_stats.shed); k += 3;
        f[k++] = 0xF7u;
        plat_sysex_send(f, k);
    } else if (p[2] == 0x02u && n >= 6u && !ui.switching) {
        uint32_t m = p[3] | (uint32_t)p[4] << 7 | (uint32_t)p[5] << 14;
        for (l = 0; l < TP_NLOOP; l++)
            if (((m >> l) & 1u) != ui.want[l])
                set_loop((int)l, (int)(m >> l & 1u));
        tp_hold(!m);                          /* playing, as PLAY would; none: back to bar 1, as stopped */
        ui.redraw |= 7;
    } else if (p[2] == 0x03u && n >= 4u && p[3] < NBK) {
        ui.pick = p[3];
        ui.pick_at = plat_ms() - 1000u;      /* now, not after the knob's rest */
    } else if (p[2] == 0x04u) {
        tp_stats.peak = tp_stats.sounding;
        pr_max = 0;
    } else if (p[2] == 0x05u && n >= 5u && p[3] < NSET && p[4] <= 100u) {
        for (l = 0; l < TP_NLOOP; l++)
            tp_param((int)l, p[3], u_value(p[3], p[4]));
    } else if (p[2] == 0x06u) {
        for (l = 0; l < TP_NLOOP; l++)
            apply_loop((int)l);
    }
    return 1;
}

#ifdef PLAT_HOST
/* an older firmware's settings, as it saved them: TDP7 (twelve banks, then the canvas), TDP6 (ten), TDP5 (five, then the canvas) or TDP3 (five banks). Marked
 * values: tempo 88; bank 1 loop 1 volume 33; the canvas's loop 1 volume 44; bank 3's progression 7; the recorder's
 * instrument 3, 4 bars, bar snap, no click, octave +1 */
int app_host_cmd(const char *cmd, const char *a, const char *b, const char *c, const char *d)
{
    static union {
        settings7_t s7;
        settings6_t s6;
        settings5_t s5;
        uint8_t raw[sizeof(settings7_t)];
    } o;
    int b0, l, k;
    (void)c;
    (void)d;
    if (strcmp(cmd, "oldsettings"))
        return 0;
    memset(&o, 0, sizeof o);
    b0 = atoi(b);
    if (atoi(a) == 7) {                      /* TDP7: twelve banks, then the canvas */
        for (k = 0; k <= OLD7_NBANK; k++)
            for (l = 0; l < TP_NLOOP; l++) {
                o.s7.u[k][l][S_VOL] = 70;
                o.s7.u[k][l][S_DEC] = 50;
            }
        o.s7.magic = SET_MAGIC7;
        o.s7.bpm = 88;
        o.s7.bank = (uint8_t)b0;
        o.s7.u[0][0][S_VOL] = 33;
        o.s7.u[11][0][S_VOL] = 55;           /* Old Blood's first loop */
        o.s7.u[OLD7_NBANK][0][S_VOL] = 44;
        o.s7.prog[2] = 7;
        o.s7.instr = 3;
        o.s7.bars = 4;
        o.s7.snap = TP_SNAP_BAR;
        o.s7.click = 0;
        o.s7.oct = 1;
        plat_store_save(OBJ_SETTINGS, &o, sizeof o.s7);
        return 1;
    }
    if (atoi(a) == 6) {                      /* TDP6: ten banks, then the canvas */
        for (k = 0; k <= OLD6_NBANK; k++)
            for (l = 0; l < TP_NLOOP; l++) {
                o.s6.u[k][l][S_VOL] = 70;
                o.s6.u[k][l][S_DEC] = 50;
            }
        o.s6.magic = SET_MAGIC6;
        o.s6.bpm = 88;
        o.s6.bank = (uint8_t)b0;
        o.s6.u[0][0][S_VOL] = 33;
        o.s6.u[9][0][S_VOL] = 55;            /* Night Cafe's first loop */
        o.s6.u[OLD6_NBANK][0][S_VOL] = 44;
        o.s6.prog[2] = 7;
        o.s6.instr = 3;
        o.s6.bars = 4;
        o.s6.snap = TP_SNAP_BAR;
        o.s6.click = 0;
        o.s6.oct = 1;
        plat_store_save(OBJ_SETTINGS, &o, sizeof o.s6);
        return 1;
    }
    for (k = 0; k <= OLD_NBANK; k++)
        for (l = 0; l < TP_NLOOP; l++) {
            o.s5.u[k][l][S_VOL] = 70;
            o.s5.u[k][l][S_DEC] = 50;
        }
    o.s5.bpm = 88;
    o.s5.bank = (uint8_t)b0;
    o.s5.u[0][0][S_VOL] = 33;
    if (atoi(a) == 5) {
        o.s5.magic = SET_MAGIC5;
        o.s5.u[OLD_NBANK][0][S_VOL] = 44;
        o.s5.prog[2] = 7;
        o.s5.instr = 3;
        o.s5.bars = 4;
        o.s5.snap = TP_SNAP_BAR;
        o.s5.click = 0;
        o.s5.oct = 1;
        plat_store_save(OBJ_SETTINGS, &o, sizeof o.s5);
    } else {
        o.s5.magic = SET_MAGIC3;
        o.raw[SET2_SIZE + 2] = 7;            /* TDP3: the five progressions after the five banks */
        plat_store_save(OBJ_SETTINGS, &o, SET3_SIZE);
    }
    return 1;
}

int app_expect(const char *what, int *got)
{
    if (!strcmp(what, "wanted")) {          /* how many loops are asked to play */
        int l;
        *got = 0;
        for (l = 0; l < TP_NLOOP; l++)
            *got += ui.want[l];
    } else if (!strncmp(what, "want", 4))    /* wantN: loop N asked to play */
        *got = ui.want[atoi(what + 4) % TP_NLOOP];
    else if (!strncmp(what, "state", 5))     /* stateN: the engine's view (1 playing, 2 joining, 4 sounding) */
        *got = tp_loop_state(atoi(what + 5) % TP_NLOOP);
    else if (!strcmp(what, "setting"))
        *got = ui.setting;
    else if (!strcmp(what, "row"))
        *got = ui.row;
    else if (!strcmp(what, "bpm"))
        *got = set.bpm;
    else if (!strcmp(what, "dirty"))
        *got = set_dirty;
    else if (!strcmp(what, "voices"))
        *got = tp_stats.sounding;
    else if (!strncmp(what, "u", 1) && what[1] >= '0' && what[1] <= '9')   /* uL_K: loop L's setting K */
        *got = USET[atoi(what + 1) % TP_NLOOP][atoi(strchr(what, '_') ? strchr(what, '_') + 1 : "0") % NSET];
    else if (!strcmp(what, "prog"))         /* the bank's chord progression */
        *got = set.prog[set.bank];
    else if (!strcmp(what, "bank"))
        *got = set.bank;
    else if (!strcmp(what, "recmode"))      /* the canvas: recording mode */
        *got = ui.rec_mode;
    else if (!strcmp(what, "rs"))           /* the recorder: 0 idle, 1 set up, 2 count-in, 3 recording */
        *got = ui.rs;
    else if (!strcmp(what, "rslot"))
        *got = ui.rslot;
    else if (!strcmp(what, "delsel"))
        *got = ui.del_sel;
    else if (!strcmp(what, "oct"))
        *got = set.oct;
    else if (!strcmp(what, "playhint"))     /* PLAY held: the chords offered */
        *got = ui.play_hint;
    else if (!strcmp(what, "texmoves"))     /* autoplay's texture swaps */
        *got = ui.auto_tex_moves;
    else if (!strcmp(what, "chordmoves"))   /* autoplay's progression changes */
        *got = ui.auto_chord_moves;
    else if (!strcmp(what, "auto"))         /* autoplay on */
        *got = ui.autoplay;
    else if (!strcmp(what, "held"))         /* the clock held at bar 1 (nothing started yet, or stopped) */
        *got = tp_held();
    else if (!strncmp(what, "rowon", 5)) {  /* rowonR: loops playing in screen row R */
        int l;
        *got = 0;
        for (l = 0; l < TP_NLOOP; l++)
            *got += ui.want[l] && pos_of(l) / 4 == atoi(what + 5) % 4;
    } else if (!strcmp(what, "snap"))         /* a new loop's snap (TP_SNAP_*) */
        *got = set.snap;
    else if (!strcmp(what, "instr"))        /* the keys' instrument */
        *got = set.instr;
    else if (!strncmp(what, "notes", 5))    /* notesN: canvas loop N's notes */
        *got = tp_user[atoi(what + 5) % TP_NLOOP].n;
    else if (!strncmp(what, "used", 4))     /* usedN: canvas loop N recorded */
        *got = tp_user[atoi(what + 4) % TP_NLOOP].used;
    else if (!strncmp(what, "linstr", 6))   /* linstrN: canvas loop N's instrument */
        *got = tp_user[atoi(what + 6) % TP_NLOOP].instr;
    else if (!strncmp(what, "lbars", 5))    /* lbarsN: its length */
        *got = tp_user[atoi(what + 5) % TP_NLOOP].bars;
    else
        return 0;
    return 1;
}
#endif
