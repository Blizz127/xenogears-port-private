#!/usr/bin/env bash
# Runner-job driver (xeno_runner_run.sh, inside xenogears-dev:24.04 at
# /xenogears-decomp): builds the port twice -- ENHANCED (as committed) and
# BASELINE (text_speed_baseline.patch applied, text_speed.c not built: the hook
# seams are then absent, i.e. the original code paths) -- and runs the text
# speed tests in ./out.  Usage: text_speed_runner_job.sh MODE...
#   (MODE: build identity speeds auto explore)
set -u
cd /xenogears-decomp
mkdir -p out
BIN=pc_port/build_native/xeno-port
if [ ! -x out/xeno-port-enhanced ]; then
    bash pc_port/build_port.sh > out/build_enhanced.log 2>&1 || { echo "enhanced build failed"; tail -30 out/build_enhanced.log; exit 1; }
    cp $BIN out/xeno-port-enhanced
fi
if [[ " $* " == *" identity "* && ! -x out/xeno-port-baseline ]]; then
    patch -p1 < pc_port/tests/text_speed_baseline.patch > out/baseline_patch.log 2>&1 || { echo "baseline patch failed"; exit 1; }
    sed -i '/pc_port\/src\/text_speed.c/d' pc_port/build_port.sh
    bash pc_port/build_port.sh > out/build_baseline.log 2>&1 || { echo "baseline build failed"; tail -30 out/build_baseline.log; exit 1; }
    cp $BIN out/xeno-port-baseline
    patch -R -p1 < pc_port/tests/text_speed_baseline.patch >> out/baseline_patch.log 2>&1
fi
export XENO_TEST_TMP=/xenogears-decomp/out/runs
rc=0
for m in "$@"; do
    case $m in
        build) ;;
        identity) XENO_PORT_BINARY=out/xeno-port-enhanced XENO_PORT_BASELINE_BINARY=out/xeno-port-baseline bash pc_port/tests/run_text_speed_test.sh identity 2>&1 | tee out/test_identity.txt || rc=1 ;;
        *) XENO_PORT_BINARY=out/xeno-port-enhanced bash pc_port/tests/run_text_speed_test.sh $m 2>&1 | tee out/test_$m.txt || rc=1 ;;
    esac
done
exit $rc
