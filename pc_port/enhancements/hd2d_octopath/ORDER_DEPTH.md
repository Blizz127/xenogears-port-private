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

God rays are a restrained depth-aware radial highlight gather toward explicit
projected light anchors. No anchor is inferred from the screen or world. Rays
require certified depth and R8 bit coverage; protected and nearer samples
attenuate shafts. This is screen-space light scattering, not volumetric world
lighting. An absent valid light anchor leaves rays disabled.

DOF uses 24 nearest samples with a circle-of-confusion-dependent radius (up to
four times the former radius at full intensity); depth discontinuities and
protected neighbours remain excluded. Bloom only gathers values above .72.
Grade uses an original mathematical eight-knot amber luminance LUT with linear
interpolation and retained chroma; no external image/LUT asset is used.

Rays use at most the first two valid supplied light anchors in caller order.
Other projected lights still contribute local lighting but no shafts. This
bounds shaft gathers to 48 samples per pixel instead of 384 at sixteen lights,
trading multiple-lamp scattering for two explicit key-light shafts. Hardware
timing must be rerun for this implementation; sample counts are not timings.

Native front-idle contact mode (`XG_HD2D_SHADOW_OT_CONTACT`) uses a complete six-part snapshot of the verified Fei190 pose. The GPU callback must observe every unchanged packet in the same raw OT slot and the current 2D projection before it offers a normalized bottom-centre bounding-box proxy. This proxy is an art-directed sprite contact, not recovered anatomical feet or physical world geometry. Unsupported poses, perspective packets, incomplete groups and changed packet lifetimes fail closed. The receiver requires map coverage and a valid OT depth no nearer than the sprite contact; sprite/UI/override pixels remain protected. CPU reuse/expiry tests and the GL nearer-receiver regression pass. The native field14 scene observed one such contact alongside all nine effects; other animation contacts remain disabled pending evidence.
