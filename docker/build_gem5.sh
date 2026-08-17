#!/bin/bash
set -euo pipefail

SRC_DIR="/artifacts/gem5-metal"

cd "$SRC_DIR"
printf 'Building gem5 + Metal (Cobalt)...\n'
./build.sh

printf 'Building serial console...\n'
cd "$SRC_DIR/util/term"
make CC=clang
