# Text speed and auto-advance (opt-in enhancement)

Two rows under the dev menu's **Options > Enhancements** (the menu is opt-in,
`cheats.dev_menu` / `XENO_DEV_MENU=1`; the values are also read from
`config.ini` without the menu):

| Row | Values | config.ini key (`[enhancements]`) |
| --- | --- | --- |
| TEXT SPEED | RETAIL (0, default), 2X, 4X, INSTANT | `text_speed__retail_2x_4x_instant_` |
| AUTO-ADVANCE | OFF (0, default), ON | `auto_advance__off_on_` |
| AUTO-ADVANCE DELAY | 1.0 S, 1.5 S (default), 2.5 S | `auto_advance_delay__1_0_1_5_2_5_s_` |

## Preservation

With TEXT SPEED = RETAIL and AUTO-ADVANCE = OFF (the defaults) the port is
retail, frame for frame: the two hooks return the retail answer (one reveal
tick, no confirm) and touch no game state. Everything below differs from retail
only with a setting on and is an enhancement, not a fix.

The hooks are weak-symbol seams in the port-only bodies of
`func_80034888` (`src/slus_006.64/system/system.c`, reveal tick of a message
window) and `func_8008004C` (`src/field/dialogue/text_box_render.c`, field
text-box pump), both under `#ifdef XENO_PC_PORT` (the matching build keeps
`INCLUDE_ASM`). Without `pc_port/src/text_speed.c` linked the original code
paths run. The design, the exact retail states used and the reasons a choice is
never confirmed are in the header comment of `pc_port/src/text_speed.c`.

## What the settings change

* **TEXT SPEED** only for field dialogue windows. 2X/4X run 2/4 retail reveal
  ticks per frame, INSTANT runs ticks until the page stops. Ticks stop at the
  first explicit script delay, at any wait (page end, message end), at a page
  clear and at the string end, so explicit delays last their retail frame count,
  waits are never skipped, and the script-visible completion signals come from
  the unchanged retail code, only sooner. The message routine plays no
  per-character sound, so there is none to stack.
* **AUTO-ADVANCE** confirms a page that has shown the "continue" arrow for the
  delay (counted in serviced vblanks, so fast-forward shortens it in real time;
  a stall, pause or menu does not count). It never confirms a choice prompt
  (opcode 0xA9: the cursor is visible, and the owner's script is not parked on
  the window-close opcode 0x9C), shops, name entry, save screens or battle
  menus. A manual press still works.

## Tests

`pc_port/tests/run_text_speed_test.sh identity|speeds|auto` (headless, needs the
disc; see `pc_port/tests/text_speed_test.py`). `XENO_TEXT_DIAG=1` logs the
dialogue state per field frame (`pc_port/src/text_diag.c`, test tooling).
