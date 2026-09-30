#!/usr/bin/env python3
"""HOOK 4 depth/coverage analysis (run_hd2d_buffers_test.sh).

usage: hd2d_buffers_analyze.py DUMPDIR OUTDIR [--label NAME]
       hd2d_buffers_analyze.py --pair HASH_A HASH_B LOAD_AT_B MIN_PAIRED

Input: the frame-N.{txt,depth,coverage,color} dumps of
pc_port/tests/hd2d_buffers_test_hooks.c. Per dumped frame:
  * flags == 3 (depth and coverage valid), every depth <= depth_max, and
    coverage == 0 exactly where depth == 0;
  * Fei (the player's HOOK 2 parts): the display pixels whose texel is
    opaque in the VRAM mirror are mapped to the window (the pixel centre;
    every window pixel of a display pixel whose 3x3 neighbourhood is opaque,
    so sub-pixel placement cannot reach the silhouette edge). Each must be
      visible  -- coverage bit 1 set (a sprite part), coverage == 1|2 (a
                  textured FT4, no HOOK 3), and depth == the expected depth
                  of a part covering it: its OT entry's ordinal in the frame,
                  numbered independently by the test's DrawOTag wrapper; or
      occluded -- coverage bit 1 clear and depth > every covering part's
                  depth (something nearer, drawn later: the easel);
    anything else (depth 0, smaller depth, Fei's depth without bit 1, bit 1
    with another depth) is an error. At least MIN_VISIBLE visible and
    MIN_OCCLUDED occluded pixels are required (Fei behind the easel);
  * background: pixels no primitive wrote (depth 0) hold the frame's
    background, the draw environment's clear colour, in the colour buffer
    (at most 0.5% exceptions), and there are at least MIN_BACKGROUND.
Writes PNG evidence for the first frame (depth, coverage, Fei classes).
--pair: the lane-absent check. Every present key seen in both hash logs has
the same frame CRC, and at least MIN_PAIRED of B's presents after the load
pair with A's.
"""
import os
import struct
import sys
import zlib
from collections import Counter

MIN_VISIBLE = 200
MIN_OCCLUDED = 200
MIN_BACKGROUND = 1000


def write_png(path, w, h, rows):
    raw = b''.join(b'\0' + r for r in rows)

    def chunk(t, data):
        return struct.pack('>I', len(data)) + t + data + struct.pack('>I', zlib.crc32(t + data) & 0xffffffff)
    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) +
                chunk(b'IDAT', zlib.compress(raw, 6)) + chunk(b'IEND', b''))


def parse_txt(path):
    head, parts = {}, []
    for line in open(path):
        kv = {}
        for tok in line.split():
            if '=' in tok:
                k, v = tok.split('=', 1)
                kv[k] = v
        if 'present' in kv:
            head = kv
        elif 'part' in kv:
            mask = set()
            if kv.get('mask', 'none') not in ('none', ''):
                for row in kv['mask'].split(';'):
                    if not row:
                        continue
                    y, spans = row.split(':')
                    for span in spans.split('/'):
                        a, b = span.split('-')
                        for x in range(int(a), int(b) + 1):
                            mask.add((x, int(y)))
            dw, dh = (int(v) for v in kv['disp'].split('x'))
            parts.append(dict(part=int(kv['part']), depth=int(kv['depth']), mask=mask, dw=dw, dh=dh,
                              ot=kv['ot'], quad=kv['quad']))
    return head, parts


def analyze_frame(dump, stem, outdir, evidence):
    head, parts = parse_txt(os.path.join(dump, stem + '.txt'))
    tw, th = int(head['width']), int(head['height'])
    ww, wh = (int(v) for v in head['window'].split('x'))
    flags, dmax = int(head['flags']), int(head['depth_max'])
    clear = tuple(int(v) for v in head['clear'].split(','))
    n = tw * th
    depth = struct.unpack('<%dH' % n, open(os.path.join(dump, stem + '.depth'), 'rb').read())
    cov = open(os.path.join(dump, stem + '.coverage'), 'rb').read()
    color = open(os.path.join(dump, stem + '.color'), 'rb').read()
    errors = []
    if flags != 3:
        errors.append(f'flags={flags}')
    if len(cov) != n or len(color) != 4 * n:
        return [f'{stem}: dump sizes'], {}
    over = sum(1 for d in depth if d > dmax)
    if over:
        errors.append(f'{over} pixels with depth > depth_max {dmax}')
    mismatch0 = sum(1 for i in range(n) if (depth[i] == 0) != (cov[i] == 0))
    if mismatch0:
        errors.append(f'{mismatch0} pixels where exactly one of depth/coverage is 0')

    # Fei: display pixel -> the parts covering it
    owners = {}
    for i, p in enumerate(parts):
        for px in p['mask']:
            owners.setdefault(px, set()).add(i)
    dw = parts[0]['dw'] if parts else 320
    dh = parts[0]['dh'] if parts else 224
    classes = Counter()
    bad = Counter()
    occ_depth, occ_cov = Counter(), Counter()
    fei_px = {}
    for (x, y), own in owners.items():
        near = set()
        interior = True
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                o = owners.get((x + dx, y + dy))
                if o is None:
                    interior = False
                else:
                    near |= o
        if not interior or not (0 <= x < dw and 0 <= y < dh):
            continue
        want = {parts[i]['depth'] for i in near}
        # every window pixel of the display pixel: its 3x3 neighbourhood is
        # opaque, so a sub-pixel placement shift stays inside Fei
        for cy in range(int(y * wh / dh), int((y + 1) * wh / dh)):
            for cx in range(int(x * ww / dw), int((x + 1) * ww / dw)):
                gy = wh - 1 - cy
                if not (0 <= cx < tw and 0 <= gy < th):
                    continue
                k = gy * tw + cx
                d, c = depth[k], cov[k]
                if (c & 1) and d in want and c == 3:
                    cls = 'visible'
                elif not (c & 1) and d > max(want):
                    cls = 'occluded'
                    occ_depth[d] += 1
                    occ_cov[c] += 1
                else:
                    cls = 'bad'
                    bad[('depth0' if d == 0 else 'shallower' if d < min(want) else
                         'fei_depth_no_sprite_bit' if d in want and not (c & 1) else
                         'sprite_bit_wrong_depth' if (c & 1) and d not in want else f'coverage_{c}')] += 1
                classes[cls] += 1
                fei_px[(cx, cy)] = cls
    if not parts:
        errors.append('no Fei parts recorded')
    if bad:
        errors.append(f'{sum(bad.values())} Fei pixels neither visible nor occluded: {dict(bad)}')
    if classes['visible'] < MIN_VISIBLE:
        errors.append(f'only {classes["visible"]} visible Fei pixels (< {MIN_VISIBLE})')
    if classes['occluded'] < MIN_OCCLUDED:
        errors.append(f'only {classes["occluded"]} occluded Fei pixels (< {MIN_OCCLUDED})')

    # background: depth 0 inside the window area
    bg = bg_other = 0
    for gy in range(min(wh, th)):
        row = gy * tw
        for x in range(min(ww, tw)):
            if depth[row + x] == 0:
                bg += 1
                if tuple(color[4 * (row + x): 4 * (row + x) + 3]) != clear:
                    bg_other += 1
    if bg < MIN_BACKGROUND:
        errors.append(f'only {bg} background (depth 0) pixels (< {MIN_BACKGROUND})')
    elif bg_other > bg * 0.005:
        errors.append(f'{bg_other}/{bg} depth-0 pixels are not the clear colour {clear}')

    stats = dict(present=int(head['present']), size=f'{tw}x{th}', window=f'{ww}x{wh}', disp=f'{dw}x{dh}',
                 depth_max=dmax, parts=len(parts), part_depths=sorted({p['depth'] for p in parts}),
                 visible=classes['visible'], occluded=classes['occluded'], bad=sum(bad.values()),
                 occluder_depths=dict(occ_depth.most_common(4)), occluder_coverage=dict(occ_cov),
                 background=bg, background_not_clear=bg_other,
                 coverage_hist=dict(sorted(Counter(cov).items())))
    if evidence:
        dv = [depth[(th - 1 - y) * tw + x] for y in range(th) for x in range(tw)]
        rows = []
        for y in range(th):
            r = bytearray()
            for x in range(tw):
                v = dv[y * tw + x] * 255 // max(1, dmax)
                r += bytes((v, v, v)) if dv[y * tw + x] else b'\x40\x00\x00'
            rows.append(bytes(r))
        write_png(os.path.join(outdir, 'depth.png'), tw, th, rows)
        pal = {0: (0, 0, 0), 1: (255, 0, 255), 2: (0, 160, 0), 3: (255, 255, 0), 4: (0, 0, 255)}
        rows = []
        for y in range(th):
            r = bytearray()
            for x in range(tw):
                c = cov[(th - 1 - y) * tw + x]
                r += bytes(pal.get(c, (255, 255, 255)))
            rows.append(bytes(r))
        write_png(os.path.join(outdir, 'coverage.png'), tw, th, rows)
        cp = {'visible': (255, 255, 0), 'occluded': (0, 128, 255), 'bad': (255, 0, 0)}
        rows = []
        for y in range(th):
            r = bytearray()
            for x in range(tw):
                k = (th - 1 - y) * tw + x
                r += bytes(cp[fei_px[(x, y)]]) if (x, y) in fei_px else bytes(color[4 * k:4 * k + 3])
            rows.append(bytes(r))
        write_png(os.path.join(outdir, 'fei_classes.png'), tw, th, rows)
    return errors, stats


def pair(a_path, b_path, load_at, min_paired):
    def load(p):
        rows = []
        for line in open(p):
            f = line.split()
            if len(f) >= 5:
                rows.append((f[1], f[2], f[3], f[4]))
        return rows
    a, b = load(a_path), load(b_path)
    ma, mb = {}, {}
    for m, rows in ((ma, a), (mb, b)):
        for path, size, crc, key in rows:
            m.setdefault((path, size, key), set()).add(crc)
    amb = {k for k in set(ma) | set(mb) if len(ma.get(k, ())) > 1 or len(mb.get(k, ())) > 1}
    common = (set(ma) & set(mb)) - amb
    bad = [k for k in common if ma[k] != mb[k]]
    after = [r for r in b[load_at:] if (r[0], r[1], r[3]) in common]
    ok = not bad and len(after) >= min_paired
    print(f'HD2D BUFFERS lane-absent presents A={len(a)} B={len(b)} paired_keys={len(common)} '
          f'mismatched_keys={len(bad)} ambiguous_keys={len(amb)} B_presents_after_load={len(b) - load_at} '
          f'paired_after_load={len(after)} (min {min_paired}) -> {"PASS" if ok else "FAIL"}')
    for k in bad[:8]:
        print(f'HD2D BUFFERS lane-absent mismatch {k} A={sorted(ma[k])} B={sorted(mb[k])}')
    return 0 if ok else 1


def main():
    if sys.argv[1] == '--pair':
        return pair(sys.argv[2], sys.argv[3], int(sys.argv[4]), int(sys.argv[5]))
    dump, outdir = sys.argv[1], sys.argv[2]
    label = sys.argv[sys.argv.index('--label') + 1] if '--label' in sys.argv else 'buffers'
    os.makedirs(outdir, exist_ok=True)
    stems = sorted(f[:-4] for f in os.listdir(dump) if f.startswith('frame-') and f.endswith('.txt'))
    if not stems:
        print(f'HD2D BUFFERS {label} overall=FAIL no dumps in {dump}')
        return 1
    failed = 0
    for i, stem in enumerate(stems):
        errors, stats = analyze_frame(dump, stem, outdir, i == 0)
        print(f'HD2D BUFFERS {label} {stem} {"ok" if not errors else "FAIL"} ' +
              ' '.join(f'{k}={v}' for k, v in stats.items()).replace(' ', ' '))
        for e in errors:
            print(f'HD2D BUFFERS {label} {stem}   error: {e}')
        failed += bool(errors)
    print(f'HD2D BUFFERS {label} frames={len(stems)} failed={failed} overall={"PASS" if not failed else "FAIL"}')
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
