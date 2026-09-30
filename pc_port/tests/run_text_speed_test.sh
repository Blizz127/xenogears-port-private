#!/usr/bin/env bash
# Runtime tests for the opt-in TEXT SPEED / AUTO-ADVANCE enhancement (needs the
# disc under disc/, ~15 min per scenario, HEADLESS: offscreen GL).  See
# text_speed_test.py.  Usage: run_text_speed_test.sh identity|speeds|auto|explore
#   XENO_PORT_BINARY           port built with text_speed.c (default build_native/xeno-port)
#   XENO_PORT_BASELINE_BINARY  port built with text_speed_baseline.patch applied and
#                              text_speed.c removed (identity only); see
#                              text_speed_runner_job.sh for how both are built.
set -u
unset DISPLAY WAYLAND_DISPLAY XAUTHORITY
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BINARY="${XENO_PORT_BINARY:-$ROOT/pc_port/build_native/xeno-port}"
BASE="${XENO_PORT_BASELINE_BINARY:-}"
MODE="${1:-}"
OUT="${XENO_TEST_TMP:-$HOME/.cache/xeno/port}/text_speed/$(date +%Y%m%d-%H%M%S)"
if [ ! -x "$BINARY" ] || [ ! -f "$ROOT/disc/disc1.bin" ]; then
    echo "TEXT SPEED $MODE overall=NOT_RUN (binary or disc missing)"
    exit 2
fi
T="$ROOT/pc_port/tests/text_speed_test.py"
case "$MODE" in
    identity)
        [ -x "$BASE" ] || { echo "TEXT SPEED identity overall=NOT_RUN (XENO_PORT_BASELINE_BINARY missing)"; exit 2; }
        exec timeout --kill-after=15 5400 python3 "$T" identity "$BASE" "$BINARY" "$ROOT" "$OUT" ;;
    speeds|auto|explore)
        exec timeout --kill-after=15 7200 python3 "$T" "$MODE" "$BINARY" "$ROOT" "$OUT" ;;
    *) echo "usage: $0 identity|speeds|auto|explore"; exit 2 ;;
esac
