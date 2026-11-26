"""Install the port's affine row implementation into a selected PsyCross source tree."""
import hashlib
from pathlib import Path
import sys
from tempfile import NamedTemporaryFile


SOURCES = Path(__file__).resolve().parents[1] / "src/renderer"

# Pinned builds select a separate replay tree for a modified SDK. Explicitly
# unpinned developer builds may encounter either previously shipped layout.
PREVIOUS = {
    "PsyX_fixed_uv.cpp": {
        "033fd3870aa1109e906ed56813fcd2ac77382ac49a32ab570d4d7d4b1d437120",
        "b3b6966a1ff638b4c34a89d15f4834355b33e90bdaa91bea849af09a2e5e86f8",
    },
    "PsyX_fixed_uv.h": {
        "4c49c5713a94ff73527a2c6765f967a01c13de82701f6a24f209e6423c7ec469",
        "6f203a43b3e56bdd192210488260e1f1821ccf7d90a47b0ba4abbf3b99e44da4",
    },
}


def install(directory):
    pending = {}
    for name, previous in PREVIOUS.items():
        source = SOURCES / name
        data = source.read_bytes()
        target = directory / name
        if target.is_symlink():
            raise SystemExit(f"Refusing to replace a symlink at {target}")
        if target.exists():
            current = target.read_bytes()
            if current == data:
                continue
            if hashlib.sha256(current).hexdigest() not in previous:
                raise SystemExit(f"Preserve local changes in {target} before replacing the UV sources")
        pending[target] = (data, source.stat().st_mode & 0o666)

    for target, (data, mode) in pending.items():
        with NamedTemporaryFile(dir=directory, delete=False) as output:
            output.write(data)
            staged = Path(output.name)
        try:
            staged.chmod(mode)
            staged.replace(target)
        finally:
            staged.unlink(missing_ok=True)


if __name__ == "__main__":
    install(Path(sys.argv[1]))
