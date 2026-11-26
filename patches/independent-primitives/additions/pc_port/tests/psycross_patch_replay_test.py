

UV = 'python3 "$ROOT/pc_port/tools/install_psycross_uv_rows.py" "$PSX/src/render" || exit 1'
        # A real source installation runs alongside the two synthetic structural repairs.
        (self.sdk / "src/render").mkdir()
        (self.sdk / "src/render/pinned.h").write_text("vendor header\n")
        shutil.copyfile(PORT / "tools/install_psycross_uv_rows.py",
                        self.port / "tools/install_psycross_uv_rows.py")
        shutil.copytree(PORT / "src/renderer", self.port / "src/renderer")
        series.extend((REPAIR, FONT, UV))
        for name in ("PsyX_fixed_uv.cpp", "PsyX_fixed_uv.h"):
            shutil.copyfile(PORT / "src/renderer" / name, self.sdk / "src/render" / name)
        out = self.replay(0, "applied=6 skipped=1 repairs=3", "skip (in baseline)  skip.patch")
        self.replay(0, "applied=6 skipped=1 repairs=3")
        for operation in (REPAIR, FONT, UV):
