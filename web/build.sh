#!/bin/sh
# Build the browser emulator's engine: web/tp.wasm, from the same sources as the host bench.
#   WASI_SDK=/path/to/wasi-sdk-34.0-arm64-macos web/build.sh
# wasi-sdk: https://github.com/WebAssembly/wasi-sdk/releases (unpack anywhere; nothing is installed).
set -e
cd "$(dirname "$0")"
: "${WASI_SDK:?set WASI_SDK to the unpacked wasi-sdk folder}"
"$WASI_SDK/bin/clang" --target=wasm32-wasip1 --sysroot="$WASI_SDK/share/wasi-sysroot" -mexec-model=reactor \
    -O2 -ffp-contract=off -std=c99 -Wall -Wextra -Werror=double-promotion -Wno-unused-parameter -DTP_METERS \
    -o tp.wasm tp_web.c ../src/tp_engine.c ../src/bank_shallows.c
echo "built web/tp.wasm ($(wc -c < tp.wasm | tr -d ' ') bytes)"
