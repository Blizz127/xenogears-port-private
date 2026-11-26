# CPU triangle texture-coordinate rows

The port builds `XenoBuildFixedUvRows` from `PsyX_fixed_uv.cpp`. It takes
three integer screen vertices with byte texture coordinates and writes
fixed-point horizontal steps, seeds and visible X bounds for each scanline.
PsyCross calls it through the declarations in `PsyX_fixed_uv.h`.

The author developed the routine separately using the declaration-only
interface and the integer input/output streams from two executables built
from host revision `8ad28b3b`. The author had no implementation source,
disassembly or game assets. The isolated source commit is
`a1671e26b0215882c8919df041807fb9b3c4c710`. Integration changed its
include path to the existing renderer header name.

| Input or result | SHA-256 |
|---|---|
| Declaration-only header | `56d07b066615b4350f33a91245c89d1872949840b1552c6efdbfa847159b46eb` |
| Independent source before renaming the include | `5f65003f5d9d9d32e674e2792d88091d3991d69fe792ecce2e3f6704cee6261e` |
| Integrated CPU source | `1375995751db59fe3587a1e0073064a2a5a1016f9d49473759eb433f1bb3aab7` |
| Integer row oracle | `f47954b0398d48026e331882406735bc530722b50fe4558713028178bf714337` |
| Prefilled-buffer oracle | `4ff5e1cbb52e114e803a6979c83b13845739a1f12d5d7ba4b52551cf926af957` |

The replacement matched the row oracle across 92,318 triangles and
1,542,226 visible rows. A Linux llvmpipe replay using the installed source
matched the original software GPU at all 69,120 RGB555 pixels of one US
Disc 2 workshop frame. The renderer submitted 645 GPU commands and called
the CPU routine 1,105 times. Forcing the returned U seeds to zero changed
45,278 pixels in that fixture.

This record covers the CPU row routine and its declarations. Retain the
applicable notices for PsyCross, the renderer, its shaders and the other
components when distributing a build.
