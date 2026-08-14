#!/bin/bash
set -euo pipefail

GEM5_DIR="/artifacts/gem5-metal"

printf 'Launching serial console...\n'
printf 'To exit, press Ctrl-C in the terminal running gem5.\n'
"$GEM5_DIR/util/term/m5term" 3456
