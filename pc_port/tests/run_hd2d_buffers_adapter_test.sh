#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
task_hd2d_tmp=$(mktemp -d)
trap 'rm -rf "$task_hd2d_tmp"' EXIT
for mode in O0 O2 UBSan; do
    checks=(-"$mode")
    if [[ "$mode" == UBSan ]]; then checks=(-O2 -fsanitize=undefined -fno-sanitize-recover=all); fi
    cc -std=c11 -Wall -Wextra -Werror "${checks[@]}" -Ipc_port/include_shim \
        pc_port/src/hd2d/hd2d_buffers_adapter.c pc_port/tests/hd2d/hd2d_buffers_adapter_test.c \
        -o "$task_hd2d_tmp/test"
    "$task_hd2d_tmp/test"
done
