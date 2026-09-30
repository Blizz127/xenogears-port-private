#!/usr/bin/env bash
# HD-2D HOOK 4 (PcPort_Hd2dFrameBuffers) depth/coverage correctness test.
#
# Relinks the finished build three ways (no rebuild):
#   A  the build's link inputs without any pc_port/src/hd2d object: no lane,
#      HOOK 4 absent (the producer never runs);
#   B  A + pc_port/tests/hd2d_buffers_test_hooks.c, a test-only lane that
#      defines HOOK 4 (so the producer runs) and records the player's HOOK 2
#      parts, linked with --wrap=ParsePrimitivesLinkedList so each part's expected
#      depth (its OT entry's ordinal in the frame) is numbered independently
#      of the producer; it reads both textures back on selected frames;
#   M  B built with -DHD2D_BUFFERS_TEST_MUTANT: the depth it reads back is
#      mapped through a reversed OT order. The analysis must FAIL on M.
# Each run boots headless, loads the field-14 quicksave (Fei behind the
# easel; read in place, never copied into the repository) through the cheat
# console, and XENO_FEI_HD2D=0 keeps Fei retail.
# pc_port/tests/hd2d_buffers_analyze.py then checks every dumped frame:
# Fei's opaque part pixels carry coverage bit 1 (and 2: textured FT4) and
# exactly their part's expected depth, or -- behind the easel -- a larger
# depth without bit 1; the background is depth 0 and shows the clear colour;
# depth <= depth_max, coverage 0 exactly where depth 0, flags 3. B's log must
# show depth_max equal to the test's own OT entry count on every frame.
# Lane absent: A runs the same scene; every present key the two hash logs
# (XENO_PRESENT_HASH_LOG) share must have the same frame CRC, so the frames
# with HOOK 4 absent are those B draws with the producer running.
#
# Needs a finished pc_port/build_port.sh (xeno-port.link-args,
# port-source.cflags). Run in xenogears-dev, headless (SDL_VIDEODRIVER=
# offscreen). Env: XENO_HD2D_BUFFERS_QS, XENO_HD2D_BUFFERS_OUTDIR,
# XENO_HD2D_BUFFERS_TIMEOUT (600 s per run).
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
OUT_BUILD=pc_port/build_native
QS="${XENO_HD2D_BUFFERS_QS:-$HOME/release-staging/xeno-work/hd2d/claude-r6-pad/fei_footprint/final/quicksave.xgqs}"
OUTDIR="${XENO_HD2D_BUFFERS_OUTDIR:-$HOME/release-staging/xeno-work/hd2d/claude-r6-pad/hook4/buffers/$(date +%Y%m%d-%H%M%S)}"
TIMEOUT="${XENO_HD2D_BUFFERS_TIMEOUT:-600}"
mkdir -p "$OUTDIR"
OUTDIR="$(cd "$OUTDIR" && pwd)"
say() { echo "HD2D BUFFERS $*" | tee -a "$OUTDIR/summary.txt"; }
for f in "$OUT_BUILD/xeno-port" "$OUT_BUILD/xeno-port.link-args" "$OUT_BUILD/port-source.cflags" "$QS" disc/disc1.bin; do
    [ -e "$f" ] || { say "overall=NOT_RUN missing=$f"; exit 2; }
done
grep -q '_xeno_hd2d_buffers' pc_port/extern/PsyCross/src/render/PsyX_render.cpp ||
    { say "overall=NOT_RUN psycross_hd2d_buffers.patch not applied"; exit 2; }
export SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-offscreen}"
unset DISPLAY WAYLAND_DISPLAY XAUTHORITY

mapfile -t LINK_ARGS < "$OUT_BUILD/xeno-port.link-args"
BASE=()
for a in "${LINK_ARGS[@]}"; do case "$a" in */obj/hd2d__*.o) ;; *) BASE+=("$a") ;; esac; done
WRAP=(-Wl,--wrap=ParsePrimitivesLinkedList)
build() { # name extra-cflags...
    local name="$1"; shift
    gcc -c pc_port/tests/hd2d_buffers_test_hooks.c $(cat "$OUT_BUILD/port-source.cflags") "$@" \
        -o "$OUTDIR/hooks_$name.o" 2>"$OUTDIR/hooks_$name.err" &&
        gcc "$OUTDIR/hooks_$name.o" "${BASE[@]}" "${WRAP[@]}" -o "$OUTDIR/xeno-port.$name" 2>"$OUTDIR/link_$name.err"
}
gcc "${BASE[@]}" -o "$OUTDIR/xeno-port.A" 2>"$OUTDIR/link_A.err" || { say "overall=FAIL could not link A"; exit 1; }
build B || { say "overall=FAIL could not build B (see $OUTDIR/hooks_B.err, link_B.err)"; exit 1; }
build M -DHD2D_BUFFERS_TEST_MUTANT || { say "overall=FAIL could not build M"; exit 1; }
hook4() { nm "$1" | awk '$NF == "PcPort_Hd2dFrameBuffers" {print $(NF-1)}'; }
say "binaries: A PcPort_Hd2dFrameBuffers=$(hook4 "$OUTDIR/xeno-port.A") (w = weak undefined)" \
    "B=$(hook4 "$OUTDIR/xeno-port.B") M=$(hook4 "$OUTDIR/xeno-port.M") quicksave=$QS outdir=$OUTDIR"
if nm --defined-only "$OUTDIR/xeno-port.A" | awk '{print $3}' | grep -qx PcPort_Hd2dFrameBuffers; then
    say "overall=FAIL A defines PcPort_Hd2dFrameBuffers"; exit 1
fi

fail=0
run() { # name stop dumpdir
    local name="$1" stop="$2" dump="${3:-}" drc
    rm -rf "$OUTDIR/dump_$name"
    [ -n "$dump" ] && mkdir -p "$dump"
    XENO_HD2D_BUFFERS_TEST_DIR="$dump" \
        python3 pc_port/tests/hd2d_buffers_drive.py "$OUTDIR/xeno-port.$name" "$QS" "$OUTDIR" "$name" \
        "$TIMEOUT" "$stop" > "$OUTDIR/drive_$name.txt" 2>&1
    drc=$?
    say "run=$name rc=$drc $(grep -h '^\[drive\]' "$OUTDIR/drive_$name.txt" | tail -1)"
    return "$drc"
}
attempt() { # name stop dumpdir: up to two tries
    run "$@" || run "$@"
}

attempt B "line:[hd2d-buffers-test] dumps complete" "$OUTDIR/dump_B" || { say "overall=FAIL B never dumped its frames"; exit 1; }
LOAD_B=$(sed -n 's/^load_at=//p' "$OUTDIR/drive_B.txt" | tail -1)
PRESENTS_B=$(wc -l < "$OUTDIR/hash_B.txt")
python3 pc_port/tests/hd2d_buffers_analyze.py "$OUTDIR/dump_B" "$OUTDIR" --label B | tee "$OUTDIR/analysis_B.txt" |
    tee -a "$OUTDIR/summary.txt"
[ "${PIPESTATUS[0]}" -eq 0 ] || fail=1
dm=$(grep -a -c 'but .* OT entries walked' "$OUTDIR/run_B.log")
say "B depth_max vs the test's own OT entry count: mismatching frames=$dm $(grep -a '\[hd2d-buffers-test\] hook4=' "$OUTDIR/run_B.log" | tail -1 | sed 's/.*\] //')"
[ "$dm" -eq 0 ] || fail=1

attempt M "line:[hd2d-buffers-test] dumps complete" "$OUTDIR/dump_M" || { say "FAIL mutant M never dumped its frames"; fail=1; }
if [ -d "$OUTDIR/dump_M" ]; then
    python3 pc_port/tests/hd2d_buffers_analyze.py "$OUTDIR/dump_M" "$OUTDIR/mutant" --label M > "$OUTDIR/analysis_M.txt"
    mrc=$?
    say "mutant (reversed OT depth): analysis rc=$mrc $(tail -1 "$OUTDIR/analysis_M.txt" | sed 's/^HD2D BUFFERS //') $(grep -m1 'error:' "$OUTDIR/analysis_M.txt" | sed 's/^HD2D BUFFERS //')"
    if [ "$mrc" -eq 0 ]; then say "FAIL the mutant was not caught"; fail=1; else say "mutant caught"; fi
fi

N=$((PRESENTS_B - LOAD_B))
attempt A "presents:$N" || { say "FAIL lane-absent run A did not reach $N presents after the load"; fail=1; }
python3 pc_port/tests/hd2d_buffers_analyze.py --pair "$OUTDIR/hash_A.txt" "$OUTDIR/hash_B.txt" "$LOAD_B" 30 |
    tee -a "$OUTDIR/summary.txt"
[ "${PIPESTATUS[0]}" -eq 0 ] || fail=1

if [ "$fail" -eq 0 ]; then say "overall=PASS outdir=$OUTDIR"; else say "overall=FAIL outdir=$OUTDIR"; fi
exit "$fail"
