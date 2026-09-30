#!/usr/bin/env python3
"""Run the unchanged HOOK-4 assertions with a labelled fixed NPC input.

The existing observed actor10 pose/transform fixtures are linked identically
into A, B and the reversed-depth mutant. No pixels or comparison samples are
removed and the original minimum field-pair count remains 30.
"""
import argparse
import os
from pathlib import Path
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--checkpoint', type=Path, required=True)
    parser.add_argument('--timeout', type=int, default=420)
    args = parser.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    flags = shlex.split((ROOT/'pc_port/build_native/port-source.cflags').read_text())
    objects = []
    for name in ('pose_fixture', 'pose_observer', 'transform_fixture'):
        target = out/(name+'.o')
        subprocess.run(['gcc', '-c', 'pc_port/tests/hd2d_field_'+name+'.c',
                        *flags, '-o', str(target)], cwd=ROOT, check=True)
        objects.append(str(target))
    wraps = ['-Wl,--wrap='+name for name in
             ('func_8001D2B0', 'func_8001DAE8', 'PcPort_FeiHd2dBegin', 'func_8001E298')]
    source = (ROOT/'pc_port/tests/run_hd2d_buffers_test.sh').read_text()
    root_line = 'ROOT="$(cd "$(dirname "$0")/../.." && pwd)"'
    marker = 'WRAP=(-Wl,--wrap=ParsePrimitivesLinkedList)'
    if source.count(root_line) != 1 or source.count(marker) != 1:
        raise RuntimeError('upstream test layout changed; review fixture integration')
    source = source.replace(root_line, 'ROOT='+shlex.quote(str(ROOT)))
    source = source.replace(marker, 'BASE+=('+shlex.join(objects+wraps)+')\n'+marker)
    script = out/'run-fixture.sh'
    script.write_text(source)
    (out/'fixture-scope.txt').write_text(
        'Observed map14 actor10 frame36 and transform A2384 applied equally '
        'to A/B/M. Full renderer inputs/outputs remain compared; minimum30 '
        'post-load keys, six buffer dumps and reversed-depth mutant unchanged.\n')
    env = dict(os.environ, XENO_HD2D_FIELD_POSE_FIXTURE='1',
               XENO_HD2D_FIELD_TRANSFORM_FIXTURE='1',
               XENO_HD2D_BUFFERS_QS=str(args.checkpoint.resolve()),
               XENO_HD2D_BUFFERS_OUTDIR=str(out),
               XENO_HD2D_BUFFERS_TIMEOUT=str(args.timeout))
    for name in ('DISPLAY', 'WAYLAND_DISPLAY', 'XAUTHORITY'):
        env.pop(name, None)
    result = subprocess.run(['bash', str(script)], cwd=ROOT, env=env)
    return result.returncode


if __name__ == '__main__':
    raise SystemExit(main())
