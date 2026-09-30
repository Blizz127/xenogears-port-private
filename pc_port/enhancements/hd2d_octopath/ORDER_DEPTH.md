The composition backend supports two explicit depth modes. Existing synthetic
positive GL_R32F view-distance fixtures remain in `XG_HD2D_DEPTH_LINEAR` mode,
with their original physical focus/fog parameters. Runtime OT-order buffers use
`XG_HD2D_DEPTH_OT_ORDER` with GL_R16 raw unsigned slot codes, sampled as normalized
R16 and multiplied by `65535 / ot_max_value`. Zero slot is invalid/background;
larger nonzero slots are nearer. The shader converts valid ordering to ordinal
distance `1 - clamp(order,0,1)`, including valid nearest distance zero.

OT order is a painter-order proxy. It does not establish physical camera depth,
world units, actor feet or a physically accurate focal plane. Separate normalized
`order_focus`, `order_range`, `order_tilt`, `order_fog_start` and `order_fog_end`
parameters control the proxy. Defaults are focus .5, range .2, fog .3 through .9.
Particles default to explicit physical positive view distance and remain disabled
with OT depth. A producer may explicitly set `particle_mode` to
`XG_HD2D_PARTICLE_OT_ORDER`; then each projected particle z is dimensionless
`1-scaled_raw_slot`, validated in [0,1] including nearest zero. No conversion is
inferred from depth mode. Invalid items are skipped; only matching units can
compose. Nearer scene order occludes a farther particle. These are ordinal dust
accents, not physically located particles. Projected shadows/lights retain their
existing verified-input contract.

Coverage-derived foot shadows are deliberately not implemented: a flattened
visible sprite mask cannot distinguish anatomical feet from head/limb islands
or an occluder-truncated lower edge. R16 order at visible pixels cannot recover
the hidden sprite extent. Rejecting nearer receiver pixels alone still permits
false contact shadows on farther map below such edges. A producer must supply
verified contact anchors or equivalent actor/contact evidence before this mode
can safely draw. No screen-y depth inference or invented feet is used.

Legacy binary GL_R8 protection remains supported. Explicit coverage-bit mode
decodes the original R8 byte: sprite 1, map 2, UI 4 and override 8. Mask 13 protects
sprite/UI/override pixels from every effect, including lighting, grade and
vignette. Map-only pixels can receive composition. Protected neighbours are
excluded from blur/glow samples so their crisp texture cannot bleed into the
environment. All effects fail closed without certified protection.

Both auxiliary dimensions zero retain the existing viewport-sized attachment
contract. Full-drawable attachments specify both positive dimensions and use
the viewport x/y as an exact texel offset. Scene colour is captured only from the
viewport. Auxiliary depth/coverage use texelFetch and nearest sampler state;
no interpolation of coverage bytes or ordinal slots occurs. Out-of-viewport
pixels remain untouched, and the backend restores all GL state it changes.

The GL regression preserves the R32F fixtures and verifies low bit coverage,
all-effect crisp protection, raw maximum-slot/zero-slot distinction, ordinal
fog direction, viewport-offset sampling and state restoration. Those fixtures
establish backend behavior; integrated producer/current-frame evidence remains
necessary before claiming runtime depth or coverage correctness.
