#!/bin/bash
set -euo pipefail

GEM5_DIR="/artifacts/gem5-metal"
COPPER_DIR="/artifacts/copper"
`
printf 'Launching Copper in gem5...\n'
"$GEM5_DIR/build/ARM/gem5.opt" \
    --listener-mode=on \
    "$GEM5_DIR/configs/example/arm/co3.py" \
    --with-pmu \
    --kernel "$COPPER_DIR/build/sys/castor" \
    --disk-image "$COPPER_DIR/build/bootdisk.img"
