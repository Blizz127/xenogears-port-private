#!/usr/bin/env python3
"""Runtime tests for the opt-in TEXT SPEED / AUTO-ADVANCE enhancement
(pc_port/src/text_speed.c).  Headless, needs the disc; ~15 min per scenario.

Every scenario is a fresh New Game run (title -> New Game -> prologue paged at
speed 5 -> Lahan field 14 at speed 1) with XENO_TEXT_DIAG=1: the port logs one
"[text-diag]" line per field frame while a dialogue box is visible, one line
per page (incl. a CRC of the text rows in VRAM, i.e. what the player sees) and
open/close lines.  Inside field 14 the test presses nothing itself: dialogue
pages are confirmed by the port's own message-relative press
(XENO_TEXT_DIAG_PRESS=K frames after the continue arrow shows), so the run does
not depend on wall-clock timing; the driver only presses Circle (every 2 s)
while no dialogue box is showing, to leave the easel close-up etc.

Usage:
  text_speed_test.py identity BASELINE_BIN ENHANCED_BIN ROOT OUTDIR
  text_speed_test.py speeds   ENHANCED_BIN ROOT OUTDIR
  text_speed_test.py auto     ENHANCED_BIN ROOT OUTDIR
  text_speed_test.py explore  BINARY ROOT OUTDIR            (one run, prints the log summary)

identity: BASELINE_BIN is built with the two behaviour seams reverted and no
  text_speed.c (text_speed_baseline.patch), ENHANCED_BIN with everything and
  both settings at their default (off).  The per-frame dialogue logs of the
  dialogues of field 14 must be identical.
speeds:   retail / 2x / 4x / instant: the same pages (same text CRC) and the
  same number of dialogues; explicit-delay frames (dl=) last their retail count;
  reveal time shrinks with the speed.
auto:     auto-advance ON (1.0 s and 2.5 s): plain pages are confirmed by the
  port after the delay ('[text-speed] auto-advance'), and no choice prompt
  (cursor visible) is ever confirmed.
"""
import os
import re
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'lib'))
from xeno_headless import CIRCLE, CROSS, UP, Port  # noqa: E402

TD = '[text-diag] '
KEY_SPEED = 'text_speed__retail_2x_4x_instant_'
KEY_AUTO = 'auto_advance__off_on_'
KEY_DELAY = 'auto_advance_delay__1_0_1_5_2_5_s_'


def write_config(out, speed=0, auto=0, delay=1):
    os.makedirs(out, exist_ok=True)
    path = os.path.join(out, 'config.ini')
    with open(path, 'w') as f:
        f.write('[enhancements]\n%s = %d\n%s = %d\n%s = %d\n'
                % (KEY_SPEED, speed, KEY_AUTO, auto, KEY_DELAY, delay))
    return path


def run(binary, root, out, name, speed=0, auto=0, delay=1, press=20, choice=0,
        closes=6, timeout=1500, enhancement_rows=True):
    """One New Game run; returns the [text-diag]/[text-speed] lines from the
    moment field 14 started loading until `closes` dialogues closed there."""
    out = os.path.join(out, name)
    cfg = write_config(out, speed, auto, delay) if enhancement_rows else None
    env = {'XENO_TEXT_DIAG': '1', 'XENO_TEXT_DIAG_PRESS': str(press),
           'XENO_TEXT_DIAG_CHOICE': str(choice), 'XENO_FIELD_POS_DIAG': '1'}
    if cfg:
        env['XENO_CONFIG'] = cfg
    port = Port(binary, root, out, env, drop=('[vm-trace]', 'POSDIAG'))
    lines = []
    try:
        if port.wait_for('FieldLoad begin field=490', 300, every=1.0,
                         action=lambda: port.pad(CROSS)) is None:
            return None, 'NOT_RUN title field not reached'
        if port.wait_for('title loop enter', 300, every=1.5,
                         action=lambda: port.pad(CIRCLE)) is None:
            return None, 'NOT_RUN title menu not reached'
        time.sleep(1.5)
        s = port.n()
        port.pad(UP)
        time.sleep(1.0)
        if port.wait_for('title confirm choice=2', 30, s, every=2.0,
                         action=lambda: port.pad(CIRCLE)) is None:
            return None, 'NOT_RUN New Game not confirmed'
        port.con('speed 5')
        if port.wait_for('FieldLoad begin field=14 ', 3000, s, every=0.25,
                         action=lambda: port.pad(CIRCLE, 2)) is None:
            return None, 'NOT_RUN field 14 not reached'
        port.con('speed 1')
        f14 = port.n()
        deadline, last_press, seen = time.time() + timeout, 0.0, 0
        while time.time() < deadline and port.proc.poll() is None:
            with port.lock:
                chunk = port.lines[f14:]
            closes_now = sum(1 for l in chunk if l.startswith(TD + 'close'))
            if closes_now >= closes:
                break
            # press only while no dialogue box has been logged for ~1.5 s
            recent = [l for l in chunk[-300:] if l.startswith(TD + 'f=')]
            now = time.time()
            if now - last_press >= 2.0 and (len(chunk) == seen or not recent):
                port.pad(CIRCLE, 8)
                last_press = now
            seen = len(chunk)
            time.sleep(1.0)
        time.sleep(1.0)
        with port.lock:
            lines = [l for l in port.lines[f14:]
                     if l.startswith(TD) or l.startswith('[text-speed]')]
    finally:
        port.stop()
    with open(os.path.join(out, 'diag.log'), 'w') as f:
        f.write('\n'.join(lines) + '\n')
    return lines, None


# ---- log parsing ---------------------------------------------------------
FRAME = re.compile(r'^\[text-diag\] f=(\d+) c=(\w+)(.*)$')
PAGE = re.compile(r'^\[text-diag\] page f=(\d+) b=(\d+) k=(\d+) off=(-?\d+) w=(\S*) vram=(\w+)')
OPEN = re.compile(r'^\[text-diag\] open f=(\d+) b=(\d+)')
CLOSE = re.compile(r'^\[text-diag\] close f=(\d+) b=(\d+)')
BOX = re.compile(r'\| b(\d) o=(-?\d+) ot=(-?\d+) fl=(\w+) k=(\d+) dl=(-?\d+) gd=(-?\d+) '
                 r'x=(-?\d+) r=(-?\d+) sb=(-?\d+) off=(-?\d+) w=(\S*) cur=(\d)/(-?\d+) st=(-?\d+)')


def dialogues(lines):
    """Split the log into dialogues: from an 'open' to its 'close' (per box).
    Each dialogue is a list of normalised per-frame records with the frame
    number made relative to the open, so runs that reach field 14 after a
    different number of frames still compare; `gs=` (hash of the whole game
    state, which holds frame-dependent counters) is dropped."""
    starts, cur, out = {}, {}, []
    for l in lines:
        m = OPEN.match(l)
        if m:
            cur[int(m.group(2))] = []
            starts[int(m.group(2))] = int(m.group(1))
            continue
        m = CLOSE.match(l)
        if m:
            b = int(m.group(2))
            if b in cur:
                out.append(cur.pop(b))
            continue
        m = FRAME.match(l)
        if m:
            f = int(m.group(1))
            for b in list(cur):
                cur[b].append(('f', f - starts[b], m.group(2), m.group(3)))
            continue
        m = PAGE.match(l)
        if m:
            b = int(m.group(2))
            if b in cur:
                cur[b].append(('page', int(m.group(1)) - starts[b], m.group(3), m.group(4),
                               m.group(5), m.group(6)))
    return out


def pages(lines):
    return [(m.group(5) and m.group(3), m.group(4), m.group(5), m.group(6))
            for m in (PAGE.match(l) for l in lines) if m]


def first_diff(a, b):
    for i, (x, y) in enumerate(zip(a, b)):
        if x != y:
            return i, x, y
    return (min(len(a), len(b)), None, None) if len(a) != len(b) else None


# ---- scenarios -----------------------------------------------------------
def test_identity(base_bin, enh_bin, root, out):
    la, err = run(base_bin, root, out, 'identity_baseline', enhancement_rows=False)
    if err:
        return [err]
    lb, err = run(enh_bin, root, out, 'identity_enhanced_off')
    if err:
        return [err]
    da, db = dialogues(la), dialogues(lb)
    n = min(len(da), len(db))
    print('IDENTITY dialogues baseline=%d enhanced-off=%d frames=%d/%d'
          % (len(da), len(db), sum(map(len, da)), sum(map(len, db))))
    fails = []
    if n == 0:
        fails.append('no dialogue logged in field 14')
    for i in range(n):
        d = first_diff(da[i], db[i])
        if d:
            fails.append('dialogue %d differs at record %d: %r vs %r' % (i, d[0], d[1], d[2]))
    if len(da) != len(db):
        fails.append('dialogue count differs')
    if any('[text-speed] auto-advance box' in l for l in lb):
        fails.append('auto-advance fired with the setting off')
    return fails


def test_speeds(enh_bin, root, out):
    names = ['retail', '2x', '4x', 'instant']
    logs = []
    for v, nm in enumerate(names):
        lines, err = run(enh_bin, root, out, 'speed_' + nm, speed=v)
        if err:
            return [err]
        logs.append(lines)
    fails = []
    base = pages(logs[0])
    n = len(base)
    print('SPEEDS retail pages=%d' % n)
    if n == 0:
        fails.append('no page logged')
    for v in (1, 2, 3):
        pg = pages(logs[v])
        same = pg[:n] == base[:n] and len(pg) >= n
        print('SPEEDS %-7s pages=%d identical_text_and_offsets=%s' % (names[v], len(pg), same))
        if not same:
            i = first_diff(base, pg)
            fails.append('%s: page list differs at %s' % (names[v], i))
    # explicit delays (dl>0) keep their retail frame count: run lengths of
    # the decrementing dl counter per dialogue.
    def delay_runs(lines):
        runs = []
        for d in dialogues(lines):
            run_len = 0
            for rec in d:
                if rec[0] != 'f':
                    continue
                m = BOX.search(rec[3])
                dl = int(m.group(6)) if m else 0
                if dl > 0:
                    run_len += 1
                elif run_len:
                    runs.append(run_len)
                    run_len = 0
        return runs
    rr = delay_runs(logs[0])
    for v in (1, 2, 3):
        r = delay_runs(logs[v])
        print('SPEEDS delay frame counts retail=%s %s=%s' % (rr, names[v], r))
        if r != rr:
            fails.append('%s: explicit delay frame counts differ' % names[v])
    # reveal time (frames from open to the first wait)
    def first_wait_frames(lines):
        out_ = []
        for d in dialogues(lines):
            for rec in d:
                if rec[0] == 'page':
                    out_.append(rec[1])
                    break
        return out_
    t = [first_wait_frames(lg) for lg in logs]
    print('SPEEDS frames open->first wait:', dict(zip(names, t)))
    for v in (1, 2, 3):
        if sum(t[v]) >= sum(t[v - 1]) and sum(t[v - 1]) > 0:
            fails.append('%s reveal is not faster than %s' % (names[v], names[v - 1]))
    return fails


def test_auto(enh_bin, root, out):
    fails = []
    for delay, secs in ((0, '1.0'), (2, '2.5')):
        lines, err = run(enh_bin, root, out, 'auto_%ss' % secs, auto=1, delay=delay, press=0,
                         choice=0)
        if err:
            return [err]
        adv = [l for l in lines if l.startswith('[text-speed] auto-advance box')]
        print('AUTO delay=%ss: %d auto-advances' % (secs, len(adv)))
        for l in adv[:5]:
            print('  ' + l)
        if not adv:
            fails.append('delay %ss: nothing was auto-advanced' % secs)
        want = {0: 60, 2: 150}[delay]
        for l in adv:
            m = re.search(r'after (\d+) vblanks', l)
            if not m or not want <= int(m.group(1)) <= want + 6:
                fails.append('delay %ss: advance not after ~%d vblanks: %s' % (secs, want, l))
        # choices: a box with the cursor shown must never be advanced while it shows
        cur_frames = [l for l in lines if FRAME.match(l) and 'cur=0/' in l]
        print('AUTO delay=%ss: frames with a choice cursor shown: %d' % (secs, len(cur_frames)))
        advanced_on_cursor = 0
        for i, l in enumerate(lines):
            if l.startswith('[text-speed] auto-advance box'):
                for p in reversed(lines[:i]):
                    if FRAME.match(p):
                        if 'cur=0/' in p:
                            advanced_on_cursor += 1
                        break
        if advanced_on_cursor:
            fails.append('delay %ss: auto-advance fired while a choice cursor was shown' % secs)
    return fails


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    mode = argv[1]
    if mode == 'identity' and len(argv) == 6:
        fails = test_identity(argv[2], argv[3], argv[4], argv[5])
    elif mode == 'speeds' and len(argv) == 5:
        fails = test_speeds(argv[2], argv[3], argv[4])
    elif mode == 'auto' and len(argv) == 5:
        fails = test_auto(argv[2], argv[3], argv[4])
    elif mode == 'explore' and len(argv) == 5:
        lines, err = run(argv[2], argv[3], argv[4], 'explore', closes=int(os.environ.get('CLOSES', 12)))
        print(err or '%d log lines' % len(lines))
        fails = [err] if err else []
    else:
        print(__doc__)
        return 2
    real = [f for f in fails if not f.startswith('NOT_RUN')]
    if fails and not real:
        print('TEXT SPEED %s overall=NOT_RUN (%s)' % (mode, fails[0]))
        return 2
    for f in fails:
        print('TEXT SPEED %s FAIL: %s' % (mode, f))
    print('TEXT SPEED %s overall=%s' % (mode, 'FAIL' if fails else 'PASS'))
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
