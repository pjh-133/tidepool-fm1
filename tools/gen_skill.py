#!/usr/bin/env python3
"""The Claude skill's reference lists, made from the firmware's own sources so they cannot drift from it:
plugin/skills/tidepool-fm1/references/banks.md (every bank: its chords and its sixteen loops by key) and
references/instruments.md (the Blank Canvas's instruments and the drum kits' keys).

  tools/gen_skill.py           write them
  tools/gen_skill.py --check   exit 1 if they are out of date (a bank, loop or instrument changed: run it again)

A new bank needs its line in MOOD below (the check says so)."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"
OUT = ROOT / "plugin" / "skills" / "tidepool-fm1" / "references"

# each bank's key and mood, in a few words (written by hand: the code has no words for them)
MOOD = {
    "Shallows": ("D minor", "under the water: soft drones, glassy arpeggios, kalimba, rain and the ocean"),
    "Deep Space": ("E minor", "drifting in space: event horizons, star bells, sonar and a lone flute"),
    "Forest Floor": ("G major", "a forest at dusk: earth hum, cello, marimba, crickets, a stream and an owl"),
    "Neon Rain": ("A minor", "a city at night: Juno pads, chord stabs, lo-fi beats, rain on glass and traffic"),
    "Dungeon Synth": ("A minor", "a castle's depths: stone organ, lost choir, harpsichord, war drums, cave drips"),
    "Aurora": ("D major", "the northern lights: glassy pads, ice bells, whale song and sleigh bells"),
    "Dunes": ("C minor", "dusk in the desert: tanpura, oud, ney flute, darbuka and a campfire"),
    "Zen Garden": ("F major", "a temple garden in spring rain: sho, koto, shakuhachi, taiko and wind chimes"),
    "Clockwork": ("G minor", "a workshop of gears: music box, celesta, reed organ, tick-tock and a cuckoo"),
    "Night Cafe": ("B flat major, swung", "late-night jazz: walking bass, Rhodes, muted trumpet, brushes, rain"),
    "Bonfire": ("D minor", "dark fantasy among ruins (in the mood of Dark Souls): cathedral organ, solo cello, "
                           "a lone soprano and a fire to rest at"),
    "Old Blood": ("F minor", "a gothic nightmare (in the mood of Bloodborne): choir, tremolo strings, a warped "
                             "music box, church bells and crows"),
}
ROW_FAM = [0, 2, 3, 1]                       # firmware/app.c: the screen's rows top to bottom, by family
NOTES = ["F", "F#", "G", "G#", "A", "A#", "B", "C", "C#", "D", "D#", "E"]
DRUM = {                                     # src/tp_instruments.c's drum sounds, in words
    "D_KICK": "kick", "D_KICK2": "deep kick", "D_SNARE": "snare", "D_RIM": "rim (side stick)", "D_CLAP": "clap",
    "D_BRUSH": "brush sweep", "D_CHAT": "closed hi-hat", "D_PHAT": "pedal hi-hat", "D_OHAT": "open hi-hat",
    "D_TOML": "low tom", "D_TOMM": "mid tom", "D_TOMH": "high tom", "D_CRASH": "crash", "D_RIDE": "ride",
    "D_SHAKER": "shaker", "D_TAMB": "tambourine", "D_COWBELL": "cowbell",
    "H_DOUM": "darbuka doum (deep centre)", "H_TEK": "darbuka tek (bright rim)", "H_KA": "darbuka ka (slap)",
    "H_FRAME": "frame drum", "H_SLAP": "frame drum slap", "H_TAIKO": "taiko", "H_SHIME": "shime (small taiko)",
    "H_CLAPPERS": "wood clappers", "H_UDU": "udu (clay pot)", "H_FCYM": "finger cymbals",
    "H_AGOGO_L": "agogo bell, low", "H_AGOGO_H": "agogo bell, high", "H_RIQ": "riq (jingles)",
}


def bank_files():
    order = re.findall(r"&(TP_\w+)", (SRC / "banks.c").read_text())
    files = {}
    for f in SRC.glob("bank_*.c"):
        m = re.search(r"^const tp_bank_t (TP_\w+)", f.read_text(), re.M)
        if m:
            files[m.group(1)] = f
    return [files[sym] for sym in order], files["TP_CANVAS"]


def bank_info(f):
    s = f.read_text()
    m = re.search(r'^const tp_bank_t \w+ = \{\s*"([^"]+)".*?\{\s*("[^}]*")\s*\}', s, re.M | re.S)
    name, fams = m.group(1), re.findall(r'"([^"]+)"', m.group(2))
    chords = re.findall(r'^\s*\{\s*"([^"]+)",\s*\d+,\s*\{', s, re.M)
    table = s[s.index("LOOPS[TP_NLOOP]"):]
    loops = re.findall(r'^\s*\{\s*"([^"]+)",\s*(\d)\s*,', table, re.M)
    assert len(loops) == 16 and len(chords) == 4 and len(fams) == 4, f
    return name, fams, chords, [n for n, _ in loops]


def banks_md():
    banks, _ = bank_files()
    out = ["# The banks", "",
           "Generated from the firmware's sources by `tools/gen_skill.py`: do not edit by hand.", "",
           "PRESETS turns through the banks in this order. On each bank the sixteen white keys are its loops, four "
           "rows of four, the same as the pools on the screen: keys 1-4 the top row, 5-8 the second, 9-12 the "
           "third, 13-16 the bottom row. The bottom row is always the textures (the scene's atmosphere: rain, "
           "wind, fire), which start quieter (25 % volume) than the other loops (70 %).", ""]
    missing = []
    for i, f in enumerate(banks, 1):
        name, fams, chords, loops = bank_info(f)
        if name not in MOOD:
            missing.append(name)
            continue
        key, mood = MOOD[name]
        out += [f"## {i}. {name}", "", f"{key}; {mood}.", "",
                f"Its own chords (progression 0): {' - '.join(chords)}.", "",
                "| Keys | Row | Loops |", "|---|---|---|"]
        for r in range(4):
            fam = ROW_FAM[r]
            out.append(f"| {r * 4 + 1}-{r * 4 + 4} | {fams[fam]} | {', '.join(loops[fam * 4:fam * 4 + 4])} |")
        out.append("")
    out += [f"## {len(banks) + 1}. Blank Canvas", "",
            "Starts empty: sixteen pools for loops you record (see canvas.md). Keys 1-16 are pools 1-16 in order, "
            "four rows of four. Each recorded loop keeps its own instrument, length and snap.", ""]
    if missing:
        sys.exit(f"tools/gen_skill.py: no MOOD line for {', '.join(missing)}: add one")
    return "\n".join(out)


def instruments_md():
    s = (SRC / "tp_instruments.c").read_text()
    web = re.findall(r'^\s*\{\s*"([^"]+)",\s*\d,', s[s.index("INSTR[19]"):s.index("tp_instr_name")], re.M)
    own = re.findall(r'\{\s*"([^"]+)",\s*\w+\s*\}', s[s.index("OWN[TP_NINSTR"):], re.M)
    names = web + own
    n = int(re.search(r"#define TP_NINSTR (\d+)", (SRC / "tp_canvas.h").read_text()).group(1))
    assert len(names) == n, (len(names), n)

    def kit(table):
        codes = re.findall(r"[DH]_\w+", s[s.index(f"static const uint8_t {table}[27]"):].split("};")[0])
        assert len(codes) == 27, table
        return [f"| {k + 1} | {NOTES[k % 12]}{3 + (k + 5) // 12} | {DRUM[c]} |" for k, c in enumerate(codes)]

    out = ["# The Blank Canvas's instruments", "",
           "Generated from the firmware's sources by `tools/gen_skill.py`: do not edit by hand.", "",
           f"In recording mode, PRESETS turns through these {n} in this order (the number is its place). The keys "
           "play the chosen instrument, softer or louder with how hard they are pressed.", "",
           "| # | Instrument |", "|---|---|"]
    out += [f"| {i + 1} | {nm} |" for i, nm in enumerate(names)]
    out += ["", "Notes on a few of them:", "",
            "- **Drum kit** and **Hand drums**: each key plays a different drum (below). OCT- and OCT+ do nothing "
            "for them (the screen says DRUMS: NONE, and KEYS shows KIT).",
            "- **Upright bass** sounds an octave below the key, so it sits in the bass without pressing OCT-.",
            "- Struck instruments (bells, plucks, keys, drums) ring out on their own; held ones (pads, organ, strings, "
            "winds, voices, leads) sound for as long as the key is held.", "",
            "## Drum kit: which key plays what", "",
            "Keys counted from the lowest (F3), black keys included: 27 keys, F3 to G5.", "",
            "| Key | Note | Drum |", "|---|---|---|"] + kit("KIT")
    out += ["", "## Hand drums: which key plays what", "", "| Key | Note | Drum |", "|---|---|---|"] + kit("HAND")
    return "\n".join(out) + "\n"


def main():
    files = {OUT / "banks.md": banks_md(), OUT / "instruments.md": instruments_md()}
    if "--check" in sys.argv:
        stale = [p.name for p, text in files.items() if not p.exists() or p.read_text() != text]
        if stale:
            sys.exit(f"the skill's {', '.join(stale)} out of date: run tools/gen_skill.py")
        print("skill references up to date")
        return
    OUT.mkdir(parents=True, exist_ok=True)
    for p, text in files.items():
        p.write_text(text)
        print(f"wrote {p.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
