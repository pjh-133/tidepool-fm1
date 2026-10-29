/* Tidepool on the FM-1: the browser emulator's audio thread. Runs the C engine (tp.wasm) one render
   quantum at a time: 128 frames, which is four of the engine's 32-sample blocks. */
class TidepoolFM1 extends AudioWorkletProcessor {
  constructor(opts) {
    super();
    const { wasm, seed } = opts.processorOptions;
    this.x = new WebAssembly.Instance(new WebAssembly.Module(wasm), {}).exports;
    this.x.tpw_init(seed >>> 0);
    this.quanta = 0;
    this.port.onmessage = (e) => {
      const m = e.data, x = this.x;
      if (m.t === 'toggle') x.tpw_toggle(m.l, m.on ? 1 : 0);
      else if (m.t === 'volume') x.tpw_volume(m.l, m.v);
      else if (m.t === 'param') x.tpw_param(m.l, m.k, m.v);
      else if (m.t === 'master') x.tpw_master(m.v);
      else if (m.t === 'bpm') x.tpw_bpm(m.v);
      else if (m.t === 'limit') x.tpw_limit(m.v);
      else if (m.t === 'reset') x.tpw_peak_reset();
    };
    const loops = [];
    for (let l = 0; l < 16; l++) loops.push({ name: this.cstr(this.x.tpw_name(l)), fam: this.x.tpw_fam(l) });
    this.port.postMessage({ t: 'hello', loops });
  }

  cstr(p) {                               /* TextDecoder isn't available on the audio thread */
    const b = new Uint8Array(this.x.memory.buffer);
    let s = '';
    while (b[p]) s += String.fromCharCode(b[p++]);
    return s;
  }

  process(inputs, outputs) {
    const out = outputs[0], x = this.x;
    const f = new Float32Array(x.memory.buffer, x.tpw_render(), 256);
    const L = out[0], R = out[1];
    for (let i = 0; i < 128; i++) {
      L[i] = f[2 * i];
      if (R) R[i] = f[2 * i + 1];
    }
    if (++this.quanta % 12 === 0) {       /* about 29 times a second: enough for the screen */
      const st = new Int32Array(x.memory.buffer, x.tpw_stats(), 8);
      const state = [], level = [];
      for (let l = 0; l < 16; l++) {
        state.push(x.tpw_state(l));
        level.push(x.tpw_level(l));
      }
      this.port.postMessage({ t: 'status', state, level, voices: st[0], peak: st[1], notes: st[2], stolen: st[3], dropped: st[4], step: st[5] });
    }
    return true;
  }
}
registerProcessor('tidepool-fm1', TidepoolFM1);
