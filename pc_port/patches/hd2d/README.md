# HD-2D lane PsyCross patches

`pc_port/build_port.sh` applies every `*.patch` here in byte-sorted order,
after all other PsyCross patches (including `psycross_hd2d_hooks.patch`).
Each patch should add the literal sentinel comment `/* _hd2d_<basename> */`
(basename without `.patch`); an already-applied patch is also skipped by a
reverse check. Generate with `diff -u` against the tree the earlier patches
produce, and check that it replays on a fresh clone (remove `pc_port/extern`
and rebuild).

`zz_producer_enabled.patch` applies after all other lane patches to the landed
HOOK4 vendor tree. Runtime production requires Hook4 presence and an optional
weak `PcPort_Hd2dEnabled()` query. Missing query preserves the owner contract's
presence-only behavior. Shader compilation and tag attribute/program locations
remain presence-only so OFF-to-ON toggles work without recompiling shaders.
The direct sprite tagging export wraps an internally renamed owner-included
function; the owner inl and public contract are unchanged. OFF presents deliver
NULL metadata without drawable queries or tag GL work. Textures remain borrowed
and allocated targets are retained on OFF. The next ON scene resets tags.

`python3 pc_port/tests/hd2d/hd2d_producer_enabled_test.py` verifies byte-exact
reverse/forward replay against `.assist/psycross-hook4-fresh`, selective runtime
site guards, retained presence-only compile setup, and the actual extracted
helper/export wrapper under OFF, ON, absent query, and absent Hook4 linkage.
This proves the guard logic without a native GPU build; integrated OFF cost
and toggle/frame correctness still require the root's native runtime gate.
