/* Tidepool on the FM-1: the Blank Canvas bank. Its 16 loops play what was recorded into them (tp_user), as the web
 * app's playUserAt: each step, the notes that fall on it, snapped if the loop asks, scheduled at their place in it. */
#include "tp_canvas.h"
#include "tp_math.h"

tp_user_t tp_user[TP_NLOOP];

void tp_user_place(const tp_user_t *U, const tp_note_t *N, int32_t *pos, int32_t *dur)
{
    int32_t len256 = (int32_t)(U->bars ? U->bars : 1) * 16 * 256, p = N->pos, d = N->dur;
    if (U->snap == TP_SNAP_BAR) {               /* the bar it is in (a beat early: the next), whole bars long */
        int32_t nb = (d + 2048) / 4096;
        p = (p + 1024) / 4096 * 4096;
        d = (nb < 1 ? 1 : nb) * 4096;
        if (d > len256)
            d = len256;
    } else if (U->snap == TP_SNAP_16 || U->snap == TP_SNAP_8) {
        int32_t q = U->snap == TP_SNAP_16 ? 256 : 512;   /* 1/16 or 1/8, in 1/256 of a step */
        p = (p + q / 2) / q * q;
    }
    if (p >= len256)
        p -= len256;
    *pos = p;
    *dur = d;
}

static void canvas_sched(int l, int s, int off)
{
    const tp_user_t *U = &tp_user[l];
    int32_t len = (int32_t)U->bars * 16, local, i;
    float sps = tp_sps();
    if (!U->used || !U->n || len <= 0)
        return;
    local = s % len;
    for (i = 0; i < U->n; i++) {
        const tp_note_t *N = &U->notes[i];
        int32_t p, dd;
        float d;
        tp_user_place(U, N, &p, &dd);
        if (p >> 8 != local)
            continue;
        d = (float)dd * (1.0f / 256.0f) * sps;
        tp_instr_note(l, off + (int)((float)(p - local * 256) * (1.0f / 256.0f) * sps * TP_SR), U->instr, N->note,
                      (float)N->vel * (1.0f / 127.0f), d > 0.05f ? d : 0.05f);
    }
}

#define LOOP(n) { "Loop " #n, (n - 1) / 4, 0.0f, 0.35f, 0.2f, 0.05f, 0, canvas_sched }
static const tp_loop_t LOOPS[TP_NLOOP] = {
    LOOP(1), LOOP(2), LOOP(3), LOOP(4), LOOP(5), LOOP(6), LOOP(7), LOOP(8),
    LOOP(9), LOOP(10), LOOP(11), LOOP(12), LOOP(13), LOOP(14), LOOP(15), LOOP(16),
};

/* the web app's canvas chords (never heard: the loops play their own notes), its four bars named as bars */
static const tp_chord_t CHORDS[4] = {
    { "Bar 1", 48, { 60, 64, 67, 71, 72 } },
    { "Bar 2", 48, { 60, 64, 67, 71, 72 } },
    { "Bar 3", 48, { 60, 64, 67, 71, 72 } },
    { "Bar 4", 48, { 60, 64, 67, 71, 72 } },
};
static const uint8_t PENTA[1] = { 60 };

const tp_bank_t TP_CANVAS = { "Blank Canvas", LOOPS, CHORDS, PENTA, 1, 1, 0.0f, { "Row 1", "Row 2", "Row 3", "Row 4" },
                              TP_STYLE_LUSH, 1 };
