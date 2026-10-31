#!/usr/bin/env python3
"""Tidepool on the FM-1: measure the firmware's audio load on the device itself, over USB-MIDI SysEx (app.c).

  fm1_bench.py                 idle, then every bank with all 16 loops on
  fm1_bench.py --bank 3        one bank (1..13)
  fm1_bench.py --solo 3        one bank, each loop on its own (which loops cost what)
  fm1_bench.py --stats         one reading of whatever is playing now
  fm1_bench.py --stress 5      a bank with every loop's attack and decay at their maximum: an overload, to watch
                               the overload guard (app_overload) shed notes instead of dropping audio

Needs mido and python-rtmidi. The FM-1 plays out loud while it runs: turn MASTER down if you like (it does not
change the load). Each reading is the average over its window, in % of the render's time:
  render   the app's whole render (the engine and its conversion to the codec's format)
  voices / mix / fx / comp   the engine's stages
  meter    fm1-base's own meter (plat_cpu_pct): the whole audio interrupt, USB audio and the nested key scan too
"""
import argparse
import sys
import time

import mido

HDR = [0x7D, 0x54]


def find_port():
    names = mido.get_input_names()
    for want in ("TIDEPOOL", "Felucca", "FM-1"):
        for n in names:
            if want.lower() in n.lower() and n in mido.get_output_names():
                return n
    sys.exit(f"no FM-1 found among the MIDI ports: {names}")


class FM1:
    def __init__(self):
        name = find_port()
        self.inp = mido.open_input(name)
        self.out = mido.open_output(name)

    def send(self, *data):
        self.out.send(mido.Message("sysex", data=HDR + list(data)))
        time.sleep(0.25)                 # the FM-1 takes one SysEx frame at a time: let it read this one first

    def stats(self):
        for _ in self.inp.iter_pending():
            pass
        self.send(0x01)
        end = time.time() + 2.0
        while time.time() < end:
            for m in self.inp.iter_pending():
                if m.type == "sysex" and list(m.data[:3]) == HDR + [0x41]:
                    d = list(m.data[3:])
                    v = [d[i] | d[i + 1] << 7 | d[i + 2] << 14 for i in range(0, len(d) - 2, 3)]
                    keys = ["render", "worst", "voices", "mix", "fx", "comp", "meter", "xruns", "sounding", "peak",
                            "bank", "mask", "shed"]
                    r = dict(zip(keys, v))
                    for k in ("render", "worst", "voices", "mix", "fx", "comp"):
                        r[k] /= 10.0
                    return r
            time.sleep(0.01)
        sys.exit("the FM-1 did not answer (is the bench build installed?)")

    def loops(self, mask):
        self.send(0x02, mask & 0x7F, mask >> 7 & 0x7F, mask >> 14 & 0x7F)

    def bank(self, b):
        self.send(0x03, b)

    def reset(self):
        self.send(0x04)


def measure(fm, settle, window):
    time.sleep(settle)
    fm.stats()                       # start a fresh window
    fm.reset()
    time.sleep(window)
    return fm.stats()


def show(label, r):
    print(f"{label:16s} render {r['render']:5.1f}%  worst {r['worst']:5.1f}%  | voices {r['voices']:5.1f}  "
          f"mix {r['mix']:4.1f}  fx {r['fx']:4.1f}  comp {r['comp']:3.1f} | meter {r['meter']:3d}%  "
          f"xruns {r['xruns']}  shed {r.get('shed', 0)}  voices now {r['sounding']:2d} peak {r['peak']:2d}", flush=True)


BANKS = ["Shallows", "Deep Space", "Forest Floor", "Neon Rain", "Dungeon Synth", "Aurora", "Dunes", "Zen Garden",
         "Clockwork", "Night Cafe", "Bonfire", "Old Blood", "Cyberpunk"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bank", type=int)
    ap.add_argument("--solo", type=int)
    ap.add_argument("--stats", action="store_true")
    ap.add_argument("--stress", type=int, help="a bank (1..13) with every loop's attack and decay at their maximum")
    ap.add_argument("--window", type=float, default=15.0)
    a = ap.parse_args()
    fm = FM1()
    if a.stats:
        show("now", fm.stats())
        return
    if a.stress:
        b = a.stress - 1
        fm.loops(0)
        time.sleep(2)
        fm.bank(b)
        time.sleep(3)
        for k in (1, 2):                 # attack and decay at their maximum, on every loop (not saved)
            fm.send(0x05, k, 100)
        fm.loops(0xFFFF)
        show(f"{BANKS[b]} stressed", measure(fm, 12, a.window))
        fm.loops(0)
        time.sleep(2)
        fm.send(0x06)                    # the saved settings again
        return
    if a.solo:
        b = a.solo - 1
        fm.loops(0)
        time.sleep(2)
        fm.bank(b)
        time.sleep(3)
        show(f"{BANKS[b]}: idle", measure(fm, 4, 6))
        for l in range(16):
            fm.loops(1 << l)
            show(f"loop {l + 1:2d}", measure(fm, 6, a.window))
            fm.loops(0)
            time.sleep(3)
        return
    banks = [a.bank - 1] if a.bank else range(13)
    fm.loops(0)
    time.sleep(2)
    show("idle", measure(fm, 4, 8))
    for b in banks:
        fm.loops(0)
        time.sleep(1.5)
        fm.bank(b)
        time.sleep(3)
        fm.loops(0xFFFF)
        show(BANKS[b], measure(fm, 12, a.window))
    fm.loops(0)


if __name__ == "__main__":
    main()
