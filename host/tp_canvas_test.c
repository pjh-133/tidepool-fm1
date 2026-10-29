/* Tidepool on the FM-1: the recorder, tested on the Mac. A 2-bar Kalimba take: eight notes on the beat during the
 * first pass, a ninth on the second pass (overdubbed), held notes released, the take finished; then the positions and
 * the playback are checked, and the whole thing written to a WAV.
 *   build/tp_canvas_test [OUT.wav] */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../src/tp_canvas.h"

#define SR 44100
static float buf[2 * 256];
static float *wav;
static long frames;

static void run(double secs)                      /* render, keeping the output */
{
    long n = (long)(secs * SR) / 256 * 256, i;
    for (i = 0; i < n; i += 256) {
        tp_render(buf, 256);
        memcpy(wav + 2 * frames, buf, sizeof buf);
        frames += 256;
    }
}

static void wav_write(const char *path)
{
    FILE *f = fopen(path, "wb");
    uint32_t sr = SR, bytes = (uint32_t)(frames * 4), u32;
    uint16_t u16;
    long i;
    fwrite("RIFF", 1, 4, f); u32 = 36 + bytes; fwrite(&u32, 4, 1, f);
    fwrite("WAVEfmt ", 1, 8, f); u32 = 16; fwrite(&u32, 4, 1, f);
    u16 = 1; fwrite(&u16, 2, 1, f); u16 = 2; fwrite(&u16, 2, 1, f);
    fwrite(&sr, 4, 1, f); u32 = sr * 4; fwrite(&u32, 4, 1, f);
    u16 = 4; fwrite(&u16, 2, 1, f); u16 = 16; fwrite(&u16, 2, 1, f);
    fwrite("data", 1, 4, f); fwrite(&bytes, 4, 1, f);
    for (i = 0; i < frames * 2; i++) {
        float x = wav[i] * 32767.0f;
        int16_t s = (int16_t)(x > 32767 ? 32767 : x < -32768 ? -32768 : x);
        fwrite(&s, 2, 1, f);
    }
    fclose(f);
}

static double rms(long from, long to)
{
    double s = 0;
    long i;
    for (i = from * 2; i < to * 2; i++)
        s += (double)wav[i] * wav[i];
    return 20 * log10(sqrt(s / (double)((to - from) * 2)) + 1e-12);
}

int main(int argc, char **argv)
{
    const double sps = 60.0 / 72.0 / 4.0;         /* 72 bpm: a step */
    static const int mel[8] = { 60, 64, 67, 72, 71, 67, 64, 62 };
    tp_rec_info_t ri;
    int fails = 0, i;
    long take_end;
    wav = calloc((size_t)SR * 2 * 120, sizeof(float));
    tp_init(&TP_CANVAS, 1);
    tp_voice_limit(32);
    tp_user[0].instr = 1;                         /* Kalimba */
    tp_user[0].bars = 2;
    tp_user[0].snap = 1;
    run(0.5);
    tp_rec_arm(0, 1);
    run(0.01);
    tp_rec_info(&ri);
    printf("armed: state %d (count-in %d), beats to go %d\n", ri.state, TP_REC_COUNTIN, ri.beats_to_go);
    while (tp_rec_info(&ri), ri.state != TP_REC_ON)   /* wait for the take to start */
        run(256.0 / SR);
    printf("take started at %.2f s\n", (double)frames / SR);
    {   /* the take's playhead: where a note played now goes in the loop. A take armed in the second half of the
         * cycle starts on the loop's bar 2 (step 16 of 32): the playhead must say so, not count from 0 */
        int32_t at;
        tp_rec_cancel();
        while (tp_step % 32 != 20)
            run(256.0 / SR);
        tp_rec_arm(0, 1);
        run(0.01);
        while (tp_rec_info(&ri), ri.state != TP_REC_ON)
            run(256.0 / SR);
        run(sps * 5);
        tp_rec_info(&ri);
        at = tp_step % 32;
        printf("take from the loop's step %d: playhead %.0f, the loop's place now %d\n", (at - 5 + 32) % 32, ri.pos, at);
        if ((int)ri.pos != at)
            fails++;
        tp_rec_cancel();                          /* then the take, starting on the loop's first step */
        while (tp_step % 32 != 4)
            run(256.0 / SR);
        tp_rec_arm(0, 1);
        run(0.01);
        while (tp_rec_info(&ri), ri.state != TP_REC_ON)
            run(256.0 / SR);
    }
    for (i = 0; i < 8; i++) {                     /* pass 1: a note on each beat, held half a beat */
        uint16_t id = tp_live_on(1, mel[i], 100);
        run(sps * 2);
        tp_live_off(id);
        run(sps * 2);
    }
    {   /* pass 2: one more note, an octave up, on beat 3 (step 8) */
        uint16_t id;
        run(sps * 8);
        id = tp_live_on(1, 84, 120);
        run(sps * 2);
        tp_live_off(id);
        run(sps * 22);
    }
    tp_rec_info(&ri);
    printf("pass now %d\n", ri.pass);
    tp_rec_finish(1);
    run(0.01);
    take_end = frames;
    tp_rec_info(&ri);
    printf("finished: state %d, notes %d\n", ri.state, tp_user[0].n);
    if (ri.state != TP_REC_OFF || tp_user[0].n != 9)
        fails++;
    for (i = 0; i < tp_user[0].n; i++) {
        const tp_note_t *N = &tp_user[0].notes[i];
        double pos = N->pos / 256.0, snapped = floor(pos + 0.5);
        printf("  note %d: pos %6.2f steps (snaps to %2.0f), dur %5.2f, pitch %d, vel %d\n", i, pos, snapped,
               N->dur / 256.0, N->note, N->vel);
        if (i < 8 && ((int)snapped % 32) != i * 4)
            fails++;                              /* each on its beat, 0, 4, 8, ... */
    }
    run(8.0);                                     /* the loop plays on (kept) */
    printf("playback level after the take: %.1f dBFS (silence would be below -90)\n", rms(take_end, frames));
    if (rms(take_end, frames) < -60)
        fails++;
    {   /* snap to the bar: a Soft pad chord played two steps before bar 2 and held only three steps starts on bar 2's
         * line and holds the whole bar */
        static const int chord[3] = { 57, 60, 64 };
        tp_user_t *U = &tp_user[1];
        tp_note_t N = { 14 * 256, 3 * 256, 0, 100 };
        int32_t pos, dur, onset = -1, k;
        double held, early;
        long at;
        tp_toggle(0, 0);
        run(6.0);                                 /* loop 1 out, its tails gone */
        U->instr = 4;
        U->bars = 2;
        U->snap = TP_SNAP_BAR;
        U->n = 0;
        for (k = 0; k < 3; k++) {
            N.note = (uint8_t)chord[k];
            U->notes[U->n++] = N;
        }
        tp_user_place(U, &U->notes[0], &pos, &dur);
        printf("bar snap: played at step 14 for 3 steps -> plays at step %d for %d steps\n", pos / 256, dur / 256);
        if (pos != 16 * 256 || dur != 16 * 256)
            fails++;
        U->used = 1;
        tp_toggle(1, 1);
        run(sps * 48);                            /* it joins on bar 2 and plays a cycle and more */
        while (tp_step % 32 != 10)                /* the last chord's release over: listen from here */
            run(256.0 / SR);
        at = frames;
        while (onset < 0 && frames - at < (long)(sps * 40 * SR)) {
            long f0 = frames;
            run(256.0 / SR);
            if (rms(f0, frames) > -50)
                onset = tp_step % 32;
        }
        early = onset;
        run(sps * 12);                            /* 12 steps into the bar: still held (three steps would have ended) */
        held = rms(frames - 2048, frames);
        printf("bar snap: the chord began at step %d of the loop; 12 steps in: %.1f dBFS\n", onset, held);
        if (early < 16 || early > 17 || held < -45)   /* the step counter is read just after the block */
            fails++;
    }
    if (argc > 1) {
        wav_write(argv[1]);
        printf("wrote %s (%.1f s)\n", argv[1], (double)frames / SR);
    }
    printf("%s\n", fails ? "FAILED" : "ok");
    return fails ? 1 : 0;
}
