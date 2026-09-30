#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
task_hd2d_settings_tmp=$(mktemp -d)
trap 'rm -rf "$task_hd2d_settings_tmp"' EXIT
for mode in O0 O2 UBSan; do
    checks=(-"$mode")
    if [[ "$mode" == UBSan ]]; then checks=(-O2 -fsanitize=undefined -fno-sanitize-recover=all); fi
    cc -std=c11 -Wall -Wextra -Werror "${checks[@]}" -Ipc_port/include_shim -Ipc_port/src \
       pc_port/src/hd2d/hd2d_bridge.c pc_port/src/hd2d/hd2d_policy.c \
       pc_port/src/hd2d/hd2d_buffers_adapter.c pc_port/tests/hd2d/hd2d_settings_test.c \
       -o "$task_hd2d_settings_tmp/test"
    env -u XENO_HD2D_OCTOPATH -u XENO_HD2D_OPENING_MAPS "$task_hd2d_settings_tmp/test"
    env -u XENO_HD2D_OCTOPATH HD2D_SETTINGS_ABSENT=1 "$task_hd2d_settings_tmp/test"
    XENO_HD2D_OCTOPATH=0 "$task_hd2d_settings_tmp/test"
    XENO_HD2D_OCTOPATH=1 "$task_hd2d_settings_tmp/test"
done
