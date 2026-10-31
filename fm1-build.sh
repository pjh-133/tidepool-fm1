#!/bin/sh
# Compile the engine for the FM-1's chip (JieLi pi32v2) with JieLi's clang, the flags fm1-base uses for an app's
# DSP, and report the sizes, what the code needs from outside, and whether any double-precision maths crept in.
# Needs Docker (the toolchain is Linux x86-64 only) and JieLi's toolchain at ~/.jieli/toolchain
# (fm1-base's tools/get_toolchain.sh fetches it). Writes build/fm1/. Nothing is installed on the FM-1.
set -e
cd "$(dirname "$0")"
TC="${JIELI_TOOLCHAIN:-$HOME/.jieli/toolchain}"
NAME=tp-jieli
mkdir -p build/fm1
if [ "$(docker inspect -f '{{.State.Running}}' $NAME 2>/dev/null)" != "true" ]; then
    docker rm -f $NAME >/dev/null 2>&1 || true
    docker run -d --rm --name $NAME --platform linux/amd64 -v "$PWD:/work" -v "$TC/:/opt/jieli:ro" -w /work \
        debian:bookworm-slim sh -c "sleep 3600" >/dev/null
fi
J="docker exec $NAME /opt/jieli"
for f in tp_engine bank_shallows bank_deepspace bank_forestfloor bank_neonrain bank_dungeon bank_aurora bank_dunes bank_zen bank_clockwork bank_cafe bank_bonfire bank_oldblood bank_cyberpunk bank_canvas banks tp_harmony tp_instruments; do
    $J/pi32v2/bin/clang -target pi32v2 -O2 -ffunction-sections -fno-builtin -mcpu=r3 -mfprev1 -ffp-contract=off \
        -std=c99 -Wall -Wextra -Wdouble-promotion -Werror=double-promotion -Wno-unused-parameter -Wno-unused-function \
        -DTP_MAXVOICE=32 -I/opt/jieli/pi32v2/include -c src/$f.c -o build/fm1/$f.o
done
$J/common/bin/objdump -h build/fm1/*.o > build/fm1/sections.txt
$J/common/bin/objdump -t build/fm1/*.o > build/fm1/symbols.txt
$J/common/bin/objdump -d -mcpu=r3 build/fm1/tp_engine.o > build/fm1/tp_engine.dis
python3 - <<'PY'
import re
size = {"code": 0, "constants": 0, "RAM": 0}
for line in open("build/fm1/sections.txt"):
    m = re.match(r"\s*\d+\s+(\S+)\s+([0-9a-f]+)\s", line)
    if not m:
        continue
    name, n = m.group(1), int(m.group(2), 16)
    if name.startswith(".text"):
        size["code"] += n
    elif name.startswith(".rodata"):
        size["constants"] += n
    elif name.startswith((".bss", ".data", "COMMON")):
        size["RAM"] += n
print("FM-1 build (pi32v2, -O2, 32 voices):")
print(f"  code {size['code']} B + constants {size['constants']} B (the FM-1's app slot: about 568 KB)")
print(f"  RAM {size['RAM']} B (the FM-1 has about 430 KB for RAM and pool together)")
syms = open("build/fm1/symbols.txt").read()
und = sorted(set(re.findall(r"\*UND\*\s+\S+\s+(\S+)$", syms, re.M)) - set(re.findall(r"\s((?:t[pv]|TP)_\w+)$", syms, re.M)))
print("  needs from outside:", ", ".join(u for u in und if u) or "nothing")
soft = sorted(set(re.findall(r"(__\w*df\w*)", syms)))
print("  double-precision routines:", ", ".join(soft) if soft else "none (single-precision FPU only)")
PY
