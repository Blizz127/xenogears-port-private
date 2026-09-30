#!/usr/bin/env python3
"""Drive the port headless into a quicksave for run_hd2d_buffers_test.sh.

usage: hd2d_buffers_drive.py BINARY QUICKSAVE OUTDIR NAME TIMEOUT_S STOP

Boots to the title, sends the cheat console `load` until the quicksave is
accepted, then runs until STOP and exits:
  STOP = "line:TEXT"     a stdout/stderr line containing TEXT (then 1 s more);
  STOP = "presents:N"    N presents logged after the load was accepted.
Every present is logged to OUTDIR/hash_NAME.txt (XENO_PRESENT_HASH_LOG), the
output to OUTDIR/run_NAME.log; the line "load_at=<present>" is printed, the
hash-log line count when the quicksave was accepted. The quicksave is read
in place. XENO_FEI_HD2D=0: Fei is drawn by retail.
"""
import os
import subprocess
import sys
import threading
import time


def count_lines(path):
    try:
        with open(path, 'rb') as f:
            return sum(1 for _ in f)
    except OSError:
        return 0


def main():
    binary, qs, out, name, timeout, stop = sys.argv[1:7]
    timeout = float(timeout)
    root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    hash_log = os.path.join(out, f'hash_{name}.txt')
    env = dict(os.environ)
    env.update(SDL_VIDEODRIVER=env.get('SDL_VIDEODRIVER', 'offscreen'), SDL_AUDIODRIVER='dummy',
               ALSOFT_DRIVERS='null', XENO_CHEATS_CONSOLE='1', XENO_QUICKSAVE_PATH=qs,
               XENO_FEI_HD2D='0', XENO_PRESENT_HASH_LOG=hash_log)
    proc = subprocess.Popen([binary], cwd=root, env=env, stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    lines = []
    log = open(os.path.join(out, f'run_{name}.log'), 'w', errors='replace')

    def reader():
        for raw in proc.stdout:
            text = raw.decode('latin1').rstrip('\n')
            lines.append(text)
            log.write(text + '\n')
            log.flush()
    threading.Thread(target=reader, daemon=True).start()
    t0 = time.time()
    state = 'boot'
    load_at = None
    rc = 1
    kind, _, arg = stop.partition(':')
    while time.time() - t0 < timeout and proc.poll() is None:
        seen = list(lines)
        if state == 'boot' and any('console reading commands' in l for l in seen) and \
                any('FieldLoad begin' in l for l in seen):
            state = 'load'
        if state == 'load':
            if any('[xeno-port][quick] loading' in l for l in seen):
                state = 'field'
                load_at = count_lines(hash_log)
                print(f'[drive] {name}: quicksave accepted at {time.time() - t0:.1f}s, present {load_at}',
                      flush=True)
            else:
                proc.stdin.write(b'load\n')
                proc.stdin.flush()
                time.sleep(3)
                continue
        if state == 'field':
            if kind == 'line' and any(arg in l for l in seen):
                time.sleep(1)
                rc = 0
                break
            if kind == 'presents' and count_lines(hash_log) - load_at >= int(arg):
                rc = 0
                break
        time.sleep(0.5)
    print(f'load_at={load_at if load_at is not None else -1}', flush=True)
    if rc:
        print(f'[drive] {name}: timeout/exit in state {state}', flush=True)
    else:
        print(f'[drive] {name}: stop condition {stop} met at {time.time() - t0:.1f}s, '
              f'{count_lines(hash_log)} presents', flush=True)
    proc.terminate()
    try:
        proc.wait(15)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()
    time.sleep(0.5)
    log.close()
    return rc


if __name__ == '__main__':
    sys.exit(main())
