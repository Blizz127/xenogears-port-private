# FT3/FT4/GT3/GT4 vertices; the renderer uses fixed-point affine rows
# for on-screen non-PGXP genuine-PS1 textured triangles. Its
apply_psycross_patch "$ROOT/pc_port/patches/psycross_opaque_quad_viewport.patch" "_xeno_oblique_quad_viewport" "unidiff-zero"
python3 "$ROOT/pc_port/tools/install_psycross_uv_rows.py" "$PSX/src/render" || exit 1
