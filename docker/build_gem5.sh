#!/bin/bash
set -euo pipefail

SRC_DIR="/artifacts/gem5-metal"

cd "$SRC_DIR"
printf 'Building gem5 + Metal (Cobalt)...\n'
scons --ignore-style build/ARM/gem5.opt -j$(nproc)

printf 'Building serial console...\n'
cd "$SRC_DIR/util/term"
make CC=clang
