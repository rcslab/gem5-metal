#!/bin/bash
set -euo pipefail

SRC_DIR="/artifacts/copper"

echo "Cleaning up..."
scons -C "$SRC_DIR" METAL=1 BUILDTYPE=PERF ARCH=arm64 -c

printf '\nBuilding Copper for AArch64 + Cobalt...\n'
scons -C "$SRC_DIR" METAL=1 BUILDTYPE=PERF ARCH=arm64
