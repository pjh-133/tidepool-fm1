/* Tidepool on the FM-1: the engine's exports for the browser emulator (web/worklet.js).
 * The same engine sources as the host bench, compiled to WebAssembly with wasi-sdk. */
#include "../src/tp_engine.h"

#define EXPORT(name) __attribute__((export_name(#name)))
#define QUANTUM 128                    /* one Web Audio render quantum: four engine blocks */

extern const tp_bank_t TP_SHALLOWS;
static float out[2 * QUANTUM];
static int32_t stats[8];

EXPORT(tpw_init) void tpw_init(uint32_t seed) { tp_init(&TP_SHALLOWS, seed); }
EXPORT(tpw_load) void tpw_load(void) { tp_init(&TP_SHALLOWS, 1); }   /* for reading the defaults on the page's thread */
EXPORT(tpw_render) float *tpw_render(void) { tp_render(out, QUANTUM); return out; }
EXPORT(tpw_toggle) void tpw_toggle(int loop, int on) { tp_toggle(loop, on); }
EXPORT(tpw_volume) void tpw_volume(int loop, float v) { tp_volume(loop, v); }
EXPORT(tpw_param) void tpw_param(int loop, int k, float v) { tp_param(loop, k, v); }
EXPORT(tpw_param_default) float tpw_param_default(int loop, int k) { return tp_param_default(loop, k); }
EXPORT(tpw_master) void tpw_master(float v) { tp_master(v); }
EXPORT(tpw_bpm) void tpw_bpm(float bpm) { tp_bpm(bpm); }
EXPORT(tpw_limit) void tpw_limit(int n) { tp_voice_limit(n); }
EXPORT(tpw_state) int tpw_state(int loop) { return tp_loop_state(loop); }
EXPORT(tpw_level) float tpw_level(int loop) { return tp_loop_level(loop); }
EXPORT(tpw_name) const char *tpw_name(int loop) { return TP_SHALLOWS.loops[loop].name; }
EXPORT(tpw_fam) int tpw_fam(int loop) { return TP_SHALLOWS.loops[loop].fam; }
EXPORT(tpw_stats) int32_t *tpw_stats(void)
{
    stats[0] = tp_stats.sounding;
    stats[1] = tp_stats.peak;
    stats[2] = (int32_t)tp_stats.started;
    stats[3] = (int32_t)tp_stats.stolen;
    stats[4] = (int32_t)tp_stats.dropped;
    stats[5] = tp_step;
    return stats;
}
EXPORT(tpw_peak_reset) void tpw_peak_reset(void) { tp_stats.peak = tp_stats.sounding; tp_stats.started = tp_stats.stolen = tp_stats.dropped = 0; }
