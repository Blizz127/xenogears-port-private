# Opening HD-2D enhancement

Owner task: `cfcfb243d32d` (supersedes the earlier HD-2D assignments). Branch: `codex/hd2d-octopath`, rebased preserving its work onto `0799d83a04f8fb381abd8c48509aa5c149f1b86b`, including the footprint, frame-buffer and numeric-menu hooks.

The live post-process hook and enhancement registry are connected. The renderer and sprite transport compile through the lane's C-source build glob. The sprite transport preserves each original packet's screen vertices, colour, UVs, ordering-table slot and link order; a lane-owned PsyCross patch associates an original packet with its own nearest-filtered replacement texture during GPU consumption. Existing Fei, input, developer-menu and transition implementations are unchanged by this lane.

Verified production maps are field14 (Fei’s house) and field1 (Lahan). The active original Fei front-idle art is authored on exact 2× retail part grids, with 24 opaque colours and binary alpha. Its six parts preserve each native quad and OT slot. Eight strict comparison triples per labelled Fei-only scene pass: all measured easel edges are 0px and 55 occluded rows are identical; Lahan’s worst part edge is 1px with a 0px whole-sprite silhouette delta. The native observed-pose replay verifies all 168 captured build presents and restoration. PNG and RGBA payload guards report zero retail byte runs. Historical art stays intact for comparison.

The expanded opening capture observes 85 raw frame indices and 137 exact variants (1,141 part keys) at all eight angles across idle, walk and run. The live package has 201 addressable frame indices; remaining script animations are unverified. Original moving-limb drafts preserve the observed order and timing. They are not production mappings: the first walking frame still has a 2px vertical error after its second artwork iteration. Coverage and zero-fallback requirements remain pending. The earlier full-cast OFF check passes 185 full-frame hashes and 24 screenshots with identical recorded NPC render inputs; organic captures expose independent NPC phase differences and are retained as failures. Fei-only strict fixtures omit NPCs equally, retain field geometry and occluders, and are explicitly distinct from full-cast OFF verification.

The registered menu path is Options ▸ Enhancements ▸ **HD-2D (Octopath style)**. Master enablement is OFF by default, with independent effect toggles. Nine intensity rows register through the landed numeric API: 0–100%, step 5. The menu owner must increase registry capacity from16 to24 and model row capacity from24 to32 to accommodate the built-in Fei row. A private candidate passes actual virtual-pad and numeric-persistence tests; agreement is pending. Without the numeric API, ten toggle rows remain available. Further integration must approve the prologue and other applicable interiors, and share one master flag between the renderer and new sprite replacement. Legacy rendering stays on its current path while this master flag is OFF.

| Effect | Controls | Required scene data |
| --- | --- | --- |
| Depth of field / tilt shift | Enable, intensity, focus and tilt | Verified linear depth or explicit OT-order proxy; sprite/UI protection |
| Bloom / glow | Enable, intensity | Sprite/UI protection |
| Warm lighting | Enable, intensity | Sprite/UI protection; projected lights; ambient works without depth |
| Sprite contact shadows | Enable, intensity | Projected feet and sprite/UI protection |
| Vignette | Enable, intensity | Current content viewport and sprite/UI protection |
| Fog | Enable, intensity | Verified linear depth or explicit OT-order proxy; sprite/UI protection |
| Particles | Enable, intensity | Projected particles, verified depth and protection |
| Colour grading | Enable, intensity | Original amber luminance LUT; current content viewport and sprite/UI protection |
| God rays | Enable, intensity | Verified depth, bit coverage and explicit projected light anchors |

Unavailable protection disables every composition effect. The backend supports explicitly distinct linear-camera-depth and normalized ordering-table modes; OT order is an ordering proxy, not physical camera depth. The latter uses R16 order and R8 coverage bits and disables physical-depth particles. See `ORDER_DEPTH.md` for units and protection semantics. The landed HOOK4 adapter consumes each current offer once and scales R16 codes by 65535/depth_max. It rejects stale dimensions, path1 and off-scope offers. CPU and synthetic GL tests pass; integrated buffer correctness and frame evidence must also pass. Sprite textures use nearest sampling. Composition effects preserve protected sprite/UI pixels, and OFF bypasses GL operations entirely.

The landed hook ABI is in `../../include_shim/hd2d_sprite.h`; the exact runtime-part compiler and packet lifetime rules are described in `PART_MAPPING.md`. The older integration proposal is retained as design history. HOOK4 supplies real depth and coverage; native scene inputs provide explicitly authored warm composition anchors and ordinal dust for maps1/14. Contact projection uses complete pre-occlusion sprite parts and current draw-time matrices; native validation is pending, so no contact-shadow success is claimed. Contacts never come from visible-mask islands. A lane patch gates the producer’s runtime work by the master flag, retaining shader compilation so OFF-to-ON toggling works. No depth or coverage texture is fabricated by this lane.

Original Fei art and candidate Alice art, with the required attribution, are in `assets/`. Alice's PNG and losslessly decoded runtime RGBA each pass the retail-byte guard with zero matches; her atlas has no live character/frame association yet. Native travel to her house stalls during field teardown, so cast mapping requires a verified checkpoint or capture from the owner. No area texture replacement, retail-derived upscaled texture, disc image or retail atlas is supplied. Areas receive the look through the renderer, subject to approved scene scope and verified scene inputs.

Standalone checks:

```sh
bash pc_port/tests/run_hd2d_test.sh
bash pc_port/tests/run_hd2d_sprite_atlas_test.sh
bash pc_port/tests/run_hd2d_bridge_test.sh
bash pc_port/tests/run_hd2d_sprite_parts_test.sh
bash pc_port/tests/run_hd2d_sprite_transport_test.sh
python3 pc_port/tests/test_compile_hd2d_atlas.py
python3 pc_port/tests/test_describe_hd2d_cells.py
```

The Python PNG authoring tests require Pillow. Runtime C does not. Tests cover Off identity, exact-frame fallback, metadata validation, protected-pixel identity, true-depth behavior, fail-closed attachments and GL state restoration.

`bash pc_port/tests/run_hd2d_benchmark.sh 1280 720 120` records the actual GL vendor/renderer and Off/all-effects median and 95th-percentile wall time, with warm-up, swap interval disabled and completion synchronized. Repeat at `1920 1080 120` on the approved hardware runner. It inherits the runner's GPU selection, uses a synthetic scene with depth/protection and sixteen lights/shadows/particles, and does not establish in-game frame rate. An integrated opening-scene performance comparison is still required.

Private baseline builds, captures, guard evidence and review packages belong in `~/release-staging/xeno-work/hd2d/`. Opening before/after comparison and a headless Alienware GPU performance run remain release requirements. Standalone success does not satisfy those in-game gates. Nothing is pushed or published.

The nine-effect synthetic benchmark with two ray anchors ran on the Alienware NVIDIA RTX5070 Ti Laptop GPU (driver615.71.09): 120 samples after30 warm-up, median/p95 0.691283/0.699928ms at1280×720 and1.542977/1.568562ms at1920×1080. The earlier eight-effect measurements are historical, with a different shader workload. These are composition timings, not in-game frame times or evidence of PsyCross batch capacity. A local native field run measured peak48 allocated PsyCross splits (4095 usable plus the sentinel), with zero capacity rejections. Integrated Alienware field timings and corresponding peak batch counts remain separate acceptance checks.

Historical 3b1f5412 validation (before the buffer integration and new art requirements): full native build and strict Psy-Q exclusion pass. Title OFF identity pairs 428 render-input keys with zero mismatches (97% scene coverage). The opt-in lane path of `run_fei_hd2d_footprint_test.sh` passes strict tol1 with six comparable triples, 288 GPU acceptances and 1,112 changed Fei pixels. All ten pinned ROM artifacts match retail. Art payload guards report zero retail byte runs. Private captures and failed diagnostic runs stay in release-staging; test fixtures are excluded from the production source glob.

Exact-grid art revision: the owner requires each original part at 2× its retail UV grid, 16–32 opaque RGB colours and binary alpha. Existing v2 is retained for comparison and is not claimed compliant. The new draft audit records 39 observed geometry/mirror variants, with unverified semantic pose associations. A text-only imagegen attempt failed exact size, palette and alpha checks and is retained as a rejected private draft. Original code-authored pixel clusters produce the verified front-idle asset and provisional animated parts without bitmap inputs or retail pixel sampling. Rejected drafts are not runtime assets.

Private packages may be launched from any working directory with `XENO_HD2D_ATLAS_DIR=/absolute/path/to/assets/hd2d`. This directory supplies the exact profile basenames; an explicit missing file fails closed and never falls back to the working directory. With no override the existing repository-relative paths remain supported.

Current checkpoint (2026-09-30): startup applies saved master, nine toggles and nine intensities from `[enhancements]` after config load. An absent master key defaults OFF; `XENO_HD2D_OCTOPATH` overrides it explicitly. The lane logs startup enabled state, atlas path and mapped frame count, each master transition, and diagnostic first-consumed/exit replacement counts. The numeric registry currently has sixteen rows; the tested capacity24/row32 proposal awaits integration so all nineteen requested rows can be exposed.

The native field14 check observed the nine-effect mask `0x1ff` on719/721 calls, with two original light anchors, one projected sprite contact and six original dust items. OFF matched185 full-frame hashes and24 screenshots; the separate title check matched154 paired keys with zero differences. The contact pass uses the original packet quad/OT order and suppresses shadows over nearer map geometry and protected sprites/UI. These checks establish this front-idle scene, not every opening map or walking sprite contact.

The animated provisional corpus now contains137 actual geometry/UV variants (85 raw frame numbers,1141 exact part keys) from naturally observed eight-way idle/walk/run inputs. All137 render-input snapshots have been cached and replayed in the diagnostic batch. Frame42/right-walk passes three strict triples with worst edge error1px. A diagnostic single triple per variant is not the final full-coverage acceptance gate; the production profile remains the independently verified front190 art. Additional animation families and opening interactions remain to be observed and verified. Provisional packs are not production replacements.

Older failed/intermediate captures were moved to the owner's private Alienware archive after SHA256 verification of all4199 files at both ends. Current evidence and owner preview sources were retained; the verified move freed4267700224 bytes locally. The private receipt records the exact archived directory names and archive digest.

Saved-settings build: native link, strict Psy-Q guard and the ten-artifact ROM gate pass. Its title OFF identity check matched241 paired input keys with zero mismatches and97% scene coverage in both runs. The137-variant animated-v3 diagnostic completed with2877/2877 exact restoration checks and6822 GPU accepts, but only3 variants met strict art bounds;134 failed and remain inactive while original vector envelopes are revised. The failure inventory is retained without pruning.

The saved-settings build also passes the standard lane footprint script with `FEI_FOOTPRINT_STRICT_HORIZONTAL=1`: six organic field14 triples, zero pixel edge discrepancies, zero occlusion discrepancies and288 measured GPU accepts. The full-cast field OFF check is separate; the ON scene script deliberately returns INCOMPLETE until its footprint/occlusion evidence is reviewed. The strict footprint result supplies that size/occlusion check for the active front190 profile, not the unverified animated pack.
