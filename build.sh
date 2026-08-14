#!/bin/sh
export CC=clang
export CXX=clang++
export LINKFLAGS_EXTRA="-fuse-ld=lld"

scons --linker=lld build/ARM/gem5.opt -j16