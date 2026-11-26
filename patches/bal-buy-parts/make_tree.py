"""Reconstruct this source checkpoint in a private Git object database."""

import argparse
import hashlib
import json
import os
from pathlib import Path
from pathlib import PurePosixPath
import subprocess
import tempfile


def git(repo, *args, data=None, env=None):
    return subprocess.run(
        ["git", "-C", repo, *args], input=data, env=env,
        check=True, stdout=subprocess.PIPE,
    ).stdout


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def read_preimage(repo, base, path, expected):
    entry = git(repo, "ls-tree", "-z", base, "--", path)
    if expected is None:
        require(not entry, f"{path} already exists in the base tree")
        return b""
    require(entry.endswith(b"\0"), f"{path} missing from the base tree")
    header, filename = entry[:-1].split(b"\t")
    mode, kind, oid = header.decode("ascii").split()
    require(filename == path.encode() and kind == "blob", f"unexpected entry for {path}")
    require(mode == expected["mode"] and oid == expected["blob"], f"preimage changed for {path}")
    data = git(repo, "cat-file", "blob", oid)
    require(len(data) == expected["size"] and digest(data) == expected["sha256"],
            f"preimage bytes changed for {path}")
    return data


def materialize(repo, base, carrier, path, edit):
    parts = PurePosixPath(path)
    require(parts.as_posix() == path and not parts.is_absolute()
            and not set(parts.parts).intersection({".git", ".", ".."}) and parts.parts,
            f"invalid project path {path}")
    old = read_preimage(repo, base, path, edit["before"])
    payload = carrier / "additions" / path
    added = payload.read_bytes() if payload.exists() else b""
    require(digest(added) == edit["added_sha256"], f"added source changed for {path}")
    result = bytearray()
    consumed = 0
    for source, offset, size in edit["spans"]:
        require(source in {"base", "added"}, f"unexpected span type for {path}")
        chunk = old if source == "base" else added
        require(type(offset) is int and type(size) is int and offset >= 0
                and size > 0 and offset + size <= len(chunk), f"invalid span for {path}")
        if source == "added":
            require(offset == consumed, f"unused or repeated added source for {path}")
            consumed += size
        result.extend(chunk[offset:offset + size])
    target = edit["after"]
    require(consumed == len(added) and len(result) == target["size"]
            and digest(result) == target["sha256"], f"reconstruction mismatch for {path}")
    require(target["mode"] in {"100644", "100755"}, f"unsupported file mode for {path}")
    require(git(repo, "hash-object", "--stdin", data=result).decode().strip() == target["blob"],
            f"unexpected Git object for {path}")
    return result


def make_tree(repo, revision):
    carrier = Path(__file__).resolve().parent
    plan = json.loads((carrier / "plan.json").read_text(encoding="utf-8"))
    require(plan["format"] == 1, "unsupported plan format")
    base = git(repo, "rev-parse", "--verify", "--end-of-options", revision + "^{tree}").decode().strip()
    files = plan["files"]
    data = {path: materialize(repo, base, carrier, path, edit) for path, edit in files.items()}
    with tempfile.TemporaryDirectory() as temp:
        env = dict(os.environ, GIT_INDEX_FILE=str(Path(temp) / "index"))
        git(repo, "read-tree", base, env=env)
        for path, content in data.items():
            target = files[path]["after"]
            oid = git(repo, "hash-object", "-w", "--stdin", data=content).decode().strip()
            git(repo, "update-index", "--add", "--cacheinfo", target["mode"], oid, path, env=env)
        tree = git(repo, "write-tree", env=env).decode().strip()
    changed = git(repo, "diff-tree", "--no-renames", "-r", "-z", "--name-only", base, tree)
    require(set(changed.decode().split("\0")[:-1]) == set(files), "unexpected paths in resulting tree")
    return tree


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("repository", type=Path)
    parser.add_argument("base", help="engineering commit or tree before this checkpoint")
    args = parser.parse_args()
    try:
        print(make_tree(args.repository, args.base))
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"{error}\n")
