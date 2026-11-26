#!/usr/bin/env bash
set -euo pipefail
ulimit -c 0
if (( $# != 3 )); then
    echo "Usage: $0 ORIGINAL_STOCKED_IRON_SELL_RAM US_FILE_10 US_FILE_12" >&2
    exit 2
fi
ram=$(realpath "$1")
program=$(realpath "$2")
companion=$(realpath "$3")
echo "4e970dcef341df8d4a39f762ef3e5295dfdea656738df7318c191e93c1c8dc82  $ram" | sha256sum -c -
echo "5a08a22f2c5cc1d131b33a944d3167a2ac460c5f9f17f2dcf94dd8860158a25c  $program" | sha256sum -c -
echo "18d35e7640a0763cdea5feaf485b8865854d0831bbf28fb3d86d0d1996f1756f  $companion" | sha256sum -c -
here=$(dirname "$(realpath "$0")")
cd "${XENO_TEST_SOURCE:-$here/../..}"
source pc_port/tests/lib/menu_retail_build.sh
sdk=${PSYCROSS_DIR:-pc_port/extern/PsyCross}
out=$(mktemp -d "${TMPDIR:-/tmp}/gear-ygg-sell.XXXXXXXX")
trap 'rm -rf "$out"' EXIT
flags=("${HX_COMMON[@]}" -DXENO_NATIVE_ONLY -fno-pie -Ipc_port/include -I"$sdk/include" -I"$sdk/include/psx")
cc=${CC:-gcc}
for mode in O2 UBSan; do
    read -ra opt <<<"$(hx_mode_flags "$mode")"
    "$cc" "${flags[@]}" "${opt[@]}" -w -c src/slus_006.64/system/system.c -o "$out/system.o"
    "$cc" "${flags[@]}" "${opt[@]}" -Wall -Wextra -Werror -no-pie -Wl,--gc-sections \
        "$here/robo_ygg_sell_category_test.c" "$out/system.o" \
        pc_port/src/{robo_menu_purchase,robo_menu_sell_editor,robo_menu_prices,robo_menu_sell_rows,robo_menu_parts_rows,robo_menu_overlay,psx_memory}.c \
        -o "$out/native"
    "$out/native" "$ram" "$program" "$companion"
    echo "$mode passed"
done
