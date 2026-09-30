#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
task_hd2d_tmp=$(mktemp -d)
trap 'rm -rf "$task_hd2d_tmp"' EXIT
for mode in O0 O2 UBSan; do
    checks=(-"$mode")
    if [[ "$mode" == UBSan ]]; then checks=(-O2 -fsanitize=undefined -fno-sanitize-recover=all); fi
    cc "${checks[@]}" -std=gnu17 -DXENO_PC_PORT -DUSE_EXTENDED_PRIM_POINTERS=0 -D_LANGUAGE_C \
        -Wall -Wextra -Werror -Ipc_port/include_shim -Iinclude \
        -Ipc_port/extern/PsyCross/include -Ipc_port/extern/PsyCross/include/psx \
        pc_port/src/hd2d/hd2d_scene_contacts.c pc_port/tests/hd2d/hd2d_scene_contacts_test.c \
        -lm -o "$task_hd2d_tmp/test"
    "$task_hd2d_tmp/test"
done
