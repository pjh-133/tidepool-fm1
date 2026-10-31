#!/usr/bin/env python3
"""Tidepool: the FM-1's own banks (src/bank_*.c) as the web app's (public/index.html), so both play the same scenes.

  bank2js.py src/bank_aurora.c ... > banks.js

Each bank's C is turned into JavaScript almost line for line: its functions keep their names and bodies, and call
the same voice API (tv_new, tv_osc, tv_filter, ...), which the web app implements on Web Audio (its tvBuild). A
loop's `l` becomes its destination node and `off` the time in seconds it starts at. Integer division is the one thing
C and JavaScript do differently: a `/` whose two sides are integer variables (any declared int in the file), integer
literals or parenthesised integer expressions is floored. Check the output's Math.floor calls against the C's
divisions by eye when a bank changes.
"""
import re
import sys

TYPES = r"(?:const\s+)?(?:unsigned\s+)?(?:int|float|uint8_t|int8_t|uint16_t|int16_t|uint32_t|int32_t|tp_voice_t|tp_chord_t)"
INT_TYPES = ("int", "uint8_t", "int8_t", "uint16_t", "int16_t", "uint32_t", "int32_t")


def split_top(s, sep=","):
    out, depth, cur = [], 0, ""
    for ch in s:
        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth -= 1
        if ch == sep and depth == 0:
            out.append(cur)
            cur = ""
        else:
            cur += ch
    out.append(cur)
    return out


def match_paren(s, i):
    """the index of the bracket closing the one at s[i]"""
    o, c = s[i], {"(": ")", "[": "]", "{": "}"}[s[i]]
    depth = 0
    for j in range(i, len(s)):
        if s[j] == o:
            depth += 1
        elif s[j] == c:
            depth -= 1
            if depth == 0:
                return j
    raise ValueError("unbalanced")


def braces_to_js(init):
    """a C initializer { 1, 2, { 3 } } as a JavaScript array"""
    return init.replace("{", "[").replace("}", "]")


def floats(s):
    return re.sub(r"\b(\d+\.\d*(?:e-?\d+)?|\d+e-?\d+|\.\d+)f\b", r"\1", s)


def casts(s):
    while True:
        m = re.search(r"\(int\)\s*", s)
        if not m:
            break
        j = m.end()
        if s[j] == "(":
            k = match_paren(s, j)
            s = s[:m.start()] + "Math.trunc" + s[j:k + 1] + s[k + 1:]
        else:
            n = re.match(r"[A-Za-z_][\w.]*(?:\[[^\]]*\])?(?:\([^()]*\))?", s[j:])
            s = s[:m.start()] + "Math.trunc(" + n.group(0) + ")" + s[j + n.end():]
    return re.sub(r"\((?:float|unsigned|uint8_t|int8_t|uint16_t|int16_t|uint32_t|int32_t)\)\s*", "", s)


def int_division(body, ints):
    """floor every a / b whose two sides are integer expressions: integer variables, literals without a point,
    and parenthesised integer expressions"""
    def is_int_operand(t):
        t = t.strip()
        if re.fullmatch(r"\d+", t):
            return True
        if re.fullmatch(r"[A-Za-z_]\w*", t):
            return t in ints
        if t.startswith("(") and t.endswith(")"):
            inner = t[1:-1]
            if "." in re.sub(r"[A-Za-z_]\w*", "", inner):   # a float literal inside
                return False
            names = re.findall(r"[A-Za-z_][\w.]*", inner)
            return all(n in ints for n in names) and not re.search(r"\b(?:sps|Math|mtof)\b", inner)
        return False

    out, i = "", 0
    pat = re.compile(r"(\([^()]*\)|[A-Za-z_]\w*|\d+)\s*/\s*(\([^()]*\)|[A-Za-z_]\w*|\d+(?:\.\d*)?)(?!\s*\*)")
    while True:
        m = pat.search(body, i)
        if not m:
            return out + body[i:]
        a, b = m.group(1), m.group(2)
        before = body[m.start() - 1] if m.start() else ""
        if is_int_operand(a) and is_int_operand(b) and before not in "*/." and not body[m.start():].startswith("//"):
            out += body[i:m.start()] + f"Math.floor({a} / {b})"
        else:
            out += body[i:m.end()]
        i = m.end()


def decls(line, ints):
    """a C declaration line as a let/const, noting its integer variables"""
    m = re.match(r"^(\s*)(static\s+)?(" + TYPES + r")\s*\*?\s*(.*);(\s*(?:/\*.*)?)$", line)
    if not m or re.match(r"^\s*(return|if|else|for|while|switch|case)\b", line):
        return line
    indent, _, typ, rest, tail = m.groups()
    is_int = any(re.search(r"\b" + t + r"\b", typ) for t in INT_TYPES)
    parts = []
    for d in split_top(rest):
        d = d.strip()
        name = re.match(r"\*?\s*(\w+)", d).group(1)
        if is_int and not d.lstrip().startswith("*"):
            ints.add(name)
        d = re.sub(r"^\*\s*", "", d)
        d = re.sub(r"^(\w+)\s*\[[^\]]*\]", r"\1", d)
        if "=" in d and "{" in d:
            k, v = d.split("=", 1)
            d = k.strip() + " = " + braces_to_js(v.strip())
        parts.append(d)
    kw = "const" if typ.startswith("const") and all("=" in p for p in parts) else "let"
    return f"{indent}{kw} {', '.join(parts)};{tail}"


def convert(path):
    src = open(path).read()
    src = re.sub(r"^#include.*\n", "", src, flags=re.M)
    src = re.sub(r"^#define SR TP_SR.*\n", "", src, flags=re.M)
    src = re.sub(r"^#define off_after.*\n", "", src, flags=re.M)
    src = re.sub(r"^#define (\w+)\s+([^/\n]+?)\s*(/\*.*)?$", lambda m: f"const {m.group(1)} = {m.group(2)};" +
                 (" " + m.group(3) if m.group(3) else ""), src, flags=re.M)
    src = floats(src)

    # the chords
    m = re.search(r"static const tp_chord_t CHORDS\[4\] = \{(.*?)\n\};", src, re.S)
    chords = []
    for name, root, notes in re.findall(r'\{\s*"([^"]+)",\s*(\d+),\s*\{([^}]*)\}\s*\}', m.group(1)):
        chords.append(f"{{name:'{name}',root:{root},notes:[{','.join(n.strip() for n in notes.split(','))}]}}")
    src = src[:m.start()] + "const CHORDS = [" + ",".join(chords) + "];" + src[m.end():]

    # the loops' table
    m = re.search(r"static const tp_loop_t LOOPS\[TP_NLOOP\] = \{(.*?)\n\};", src, re.S)
    loops = []
    for row in re.findall(r"\{\s*(\"[^\"]+\".*?)\},", m.group(1)):
        f = [x.strip() for x in split_top(row)]
        name, fam, pan, rev, dly, fade, start, sched = f
        o = f"{{name:{name.replace(chr(34), chr(39))},fam:{fam},pan:{pan},rev:{rev},dly:{dly},fadeIn:{fade}"
        if start != "0":
            o += f",start:tvStart({start})"
        if sched != "0":
            o += f",sched:tvSched({sched})"
        loops.append(o + "}")
    src = src[:m.start()] + src[m.end():]

    # the bank
    m = re.search(r"const tp_bank_t (\w+) = \{(.*?)\};", src, re.S)
    f = [x.strip() for x in split_top(m.group(2))]
    bank = dict(name=f[0].strip('"'), penta=f[3], seed=int(f[5]), dens=float(f[6]),
                fams=[x.strip().strip('"') for x in f[7].strip("{} ").split(",")], cid=m.group(1))
    src = src[:m.start()] + src[m.end():]

    # functions and their bodies
    out, ints = [], set()                     # every integer variable's name, in any function: checked by eye after
    lines = src.split("\n")
    i = 0
    while i < len(lines):
        line = lines[i]
        fm = re.match(r"^static\s+[\w\s\*]+?\b(\w+)\(([^)]*)\)\s*(/\*.*)?$", line)
        if fm:
            params = [re.findall(r"\w+", p)[-1] for p in fm.group(2).split(",") if p.strip() and p.strip() != "void"]
            ints |= {p for p, full in zip(params, fm.group(2).split(",")) if re.search(r"\bint\b", full)} - {"l", "off"}
            out.append(f"function {fm.group(1)}({', '.join(params)})" + (" " + fm.group(3) if fm.group(3) else ""))
            i += 1
            continue
        # a static const array, maybe over several lines
        am = re.match(r"^(\s*)static const \w+ (\w+)\[[^\]]*\] = (\{.*)$", line)
        if am:
            text = am.group(3)
            while text.count("{") > text.count("}"):
                i += 1
                text += "\n" + lines[i]
            out.append(f"{am.group(1)}const {am.group(2)} = {braces_to_js(text.rstrip(';'))};")
            i += 1
            continue
        line = decls(line, ints)
        out.append(line)
        i += 1
    js = "\n".join(out)
    js = casts(js)
    js = js.replace("->", ".")
    js = re.sub(r"\btp_rand\(\)", "Math.random()", js)
    js = re.sub(r"\btp_mtof\(", "mtof(", js)
    js = re.sub(r"\btp_sps\(\)", "sps()", js)
    js = re.sub(r"\btp_chord_at\(", "chordAt(", js)
    js = re.sub(r"\btp_kal\[", "KAL[", js)
    js = re.sub(r"\bSR\b", "1", js)
    js = int_division(js, ints)
    left = re.findall(r"\b(?:tp_\w+(?<!tp_burst)|uint\w*|int8_t|static)\b", js)
    if left:
        sys.exit(f"{path}: not converted: {sorted(set(left))}")
    loops_js = ",\n  ".join(loops)
    return bank, f"""(()=>{{
{js.strip()}
return{{penta:{bank['penta']},chords:CHORDS,loops:()=>[
  {loops_js}]}};
}})()"""


if __name__ == "__main__":
    for p in sys.argv[1:]:
        bank, js = convert(p)
        print(f"/* {bank['name']}: from fm1-port/{p.split('fm1-port/')[-1]} (tools/bank2js.py) */")
        print(f"const {bank['cid']}={js};")
        print(f"/* {bank['cid']} name={bank['name']!r} seed={bank['seed']} dens={bank['dens']} fams={bank['fams']} */")
