#!/usr/bin/env bash
set -euo pipefail
ulimit -c 0
if (( $# != 1 )); then
    echo "Usage: $0 ORIGINAL_FILE08_SELL_WEAPONS_RUN" >&2
    exit 2
fi
captures=$(realpath "$1")
here=$(dirname "$(realpath "$0")")
cd "${XENO_TEST_SOURCE:-$here/../..}"
source pc_port/tests/lib/menu_retail_build.sh
sdk=${PSYCROSS_DIR:-pc_port/extern/PsyCross}
cc=${CC:-gcc}
out=$(mktemp -d "${TMPDIR:-/tmp}/gear-ygg-sell-yaw.XXXXXXXX")
trap 'rm -rf "$out"' EXIT
# Input 1 is the +10 VBlank observation from an ordinary Enter Sell Weapons.
# The following four observations are spaced seven VBlanks apart. Repeat
# for the original accepted-sale departure, with the camera turning back.
hashes=(
    8181f4792a7929372623d19baf8e77ffb8910a1ab57ff56dcfb7a5454e872c94
    1ad8f92e5a0fa911b19e6428ff770efb067e723f9693f14307218a67dda4fb1d
    080c821b3466ef6ce9541860d6ae040fb62f12ef7542d1f094e0caae7de7251c
    c048069f9b7e9e51d342b6a44299d3f87666b398bfb3fc6a0a05d91cb03bbf34
    69f425216374a50c708af384f1c43b54b8d55f065b40e364e1063b7b2d8836e6
    7935b4f9d839f220fcc935e4ef7e35343590171e4632fb106affc6638c453b0f
    6eef82ea9e0fcc30e2512ecb889fd2ac9761174ffe01e4f9be90ae58de831753
    fa19b0d4da1d72749b2347c643859b0949c8c97af808005629711b4a36506ca9
    4ec0a19a0516f550b8d44983c14d60d793215722044412c9bcbaaf1a37c9af87
    4e9654042fd5a6013a5450539a8d61ade65aa9ac2c1a82fadd7b74f558198184
)
inputs=()
for action in enter_stocked_weapons accept_one_800G_sale; do
    for sample in 1 2 3 4 5; do
        inputs+=("$captures/trace_${action}_${sample}.ram")
    done
done
"$cc" -std=gnu17 -O2 -Wall -Wextra -Werror -Ipc_port/src \
    "$here/robo_ygg_sell_yaw_oracle.c" pc_port/src/battle_mips_adapter.c -o "$out/oracle"
for i in "${!hashes[@]}"; do
    file=${inputs[i]}
    echo "${hashes[i]}  $file" | sha256sum -c -
    env -u XENO_DISABLE_CPU_INTERPRETER "$out/oracle" "$file" "$out/next-$i.ram"
done
for mode in O0 O2 UBSan; do
    read -ra opt <<<"$(hx_mode_flags "$mode")"
    "$cc" -I"$sdk/include" -I"$sdk/include/psx" "${HX_COMMON[@]}" "${opt[@]}" \
        -Ipc_port/include -DXENO_NATIVE_ONLY -Wall -Wextra -Werror -fno-pie -no-pie -Wl,--gc-sections \
        "$here/robo_ygg_sell_yaw_test.c" -o "$out/native"
    for i in "${!hashes[@]}"; do
        XENO_DISABLE_CPU_INTERPRETER=1 "$out/native" \
            "${inputs[i]}" "$out/next-$i.ram"
    done
    echo "PASS carried Sell Weapons yaw in $mode"
done
