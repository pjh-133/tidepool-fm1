#!/bin/sh
# Build the host bench. Flags as the FM-1 firmwares' host builds: -O2, no fused multiply-add.
# Auto-vectorisation is off too: the FM-1's core has no SIMD, so NEON would flatter the host count.
set -e
cd "$(dirname "$0")"
mkdir -p build
CC="${CC:-cc}"
ENGINE_FLAGS="-O2 -ffp-contract=off -fno-vectorize -fno-slp-vectorize -std=c99 -Wall -Wextra -Werror=double-promotion -Wno-unused-parameter"
$CC $ENGINE_FLAGS -c src/tp_engine.c -o build/tp_engine.o
for f in bank_shallows bank_deepspace bank_forestfloor bank_neonrain bank_dungeon bank_aurora bank_dunes bank_zen bank_clockwork bank_cafe bank_bonfire bank_oldblood bank_cyberpunk bank_canvas banks tp_harmony tp_instruments; do
    $CC $ENGINE_FLAGS -c src/$f.c -o build/$f.o
done
$CC -O2 -std=c99 -Wall -Wextra -Wno-unused-parameter -c host/tp_bench.c -o build/tp_bench.o
$CC -o build/tp_bench build/tp_bench.o build/tp_engine.o build/bank_*.o build/banks.o build/tp_harmony.o build/tp_instruments.o -lm
echo "built build/tp_bench"
