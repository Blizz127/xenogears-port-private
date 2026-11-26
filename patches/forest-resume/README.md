# Disc 2 forest checkpoints

These private engine changes resume the forest controller after a cold load and check its handback before restoring the saved player pose. The accompanying tests exercise checkpoint identity and refusal boundaries. A test that reads the original field script takes it from the player’s local game data.

## Apply the patches

Use a clean private engine checkout at `af746eead91bbe5725fe2d4c415f51dbe7a7ea9d`. Set `patch_dir` to the absolute path of this directory. These commands verify the complete file tree after each patch with the starting checkout’s copy of `HANDOVER.md`.

```sh
test "$(git rev-parse HEAD)" = af746eead91bbe5725fe2d4c415f51dbe7a7ea9d
test -z "$(git status --porcelain)"
git apply --unidiff-zero --index --check "$patch_dir/forest-resume.patch"
git apply --unidiff-zero --index "$patch_dir/forest-resume.patch"
test "$(git write-tree)" = 2302b9c338a7452046c6a80f8d559e63a3f48b21
git apply --unidiff-zero --index --check "$patch_dir/forest-resume-tests.patch"
git apply --unidiff-zero --index "$patch_dir/forest-resume-tests.patch"
test "$(git write-tree)" = dfd67676891d9b6daaea3e69518cb1a02a416cc1
git apply --unidiff-zero --index --check "$patch_dir/forest-ordered-handback.patch"
git apply --unidiff-zero --index "$patch_dir/forest-ordered-handback.patch"
test "$(git write-tree)" = 06a2465966c719539927b45aac195f2dc88cecfe
```

Engineering commit `5559cdc3b9298c3181966049c3119dbef3675572` updated only `HANDOVER.md` between the focused tests and the ordered handback correction. The source and tests are already complete after the sequence above. To reproduce the recorded engineering tree including that documentation, run these additional commands in a checkout that has the documentation commit.

```sh
git restore --source=5559cdc3b9298c3181966049c3119dbef3675572 --staged --worktree -- HANDOVER.md
test "$(git write-tree)" = 8f1f4143183919e86ac5fde81b14eb15fdfb15a1
```

| Patch | SHA-256 | Private engineering commit |
| --- | --- | --- |
| forest-resume.patch | `9e2fb182b7af46a9ca0e661a1f04c820b890c9c7c1925877b11e16029d863d8e` | `35c01e13c31bebe99c4280dff1c98f7206940351` |
| forest-resume-tests.patch | `9b0a5effc100273fc78dcd36c13e1ff0bbd896b2f3e7e129b51511d60ef3f2b6` | `370e10b9d6cde39f7362012f7a792fe5129d6cfe` |
| forest-ordered-handback.patch | `1bb20827a5ef027c3595c59d9d27b1302572d9630f413f33e9edf902c45a7cc8` | `4235be4dce40f62f67570869ea636ad2b39f2162` |

## Install the private repository guard

Before pushing from this standalone private topic checkout, run the following command from its repository root. It installs the repository’s existing retail-file guard into that checkout.

```sh
sh scripts/install-retail-guard.sh
```
