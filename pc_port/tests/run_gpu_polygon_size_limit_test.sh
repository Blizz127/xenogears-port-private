#!/usr/bin/env bash
# The PS1 GPU polygon size limit: rule cases, and that the PsyCross patch
# routes all eight polygon types (F3 FT3 G3 GT3, F4 FT4 G4 GT4) through it.
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT=$(mktemp -d /tmp/gpusize.XXXXXX)
trap 'rm -rf "$OUT"' EXIT
gcc -std=c99 -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all \
    -Ipc_port/src pc_port/tests/gpu_polygon_size_limit_test.c -o "$OUT/t"
"$OUT/t"
P=pc_port/patches/psycross_gpu_polygon_size_limit.patch
tri=$(grep -c '^+.*XenoGpuSizeLimitTriangle(firstVertex' "$P")
quad=$(grep -c '^+.*XenoGpuSizeLimitQuad(firstVertex' "$P")
[ "$tri" = 4 ] && [ "$quad" = 4 ] || { echo "FAIL: patch wires $tri triangle / $quad quad sites (want 4/4)"; exit 1; }
if [ -f pc_port/extern/PsyCross/src/gpu/PsyX_GPU.cpp ]; then
    n=$(grep -c 'XenoGpuSizeLimit\(Triangle\|Quad\)(firstVertex' pc_port/extern/PsyCross/src/gpu/PsyX_GPU.cpp)
    [ "$n" = 8 ] || { echo "FAIL: vendored PsyCross has $n size-limit call sites (want 8)"; exit 1; }
fi
echo "GPU POLYGON SIZE LIMIT PASS (rule + 8 polygon types wired)"
