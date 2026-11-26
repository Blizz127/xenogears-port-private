# Disc 2 forest checkpoints

These private engine changes resume the forest controller after a cold load and check its handback before restoring the saved player pose. New checkpoints also preserve the earned follower and camera state. The accompanying tests exercise checkpoint identity and refusal boundaries. The last, test-only patch updates the estate-camera fixture and exercises the forest path when the user supplies Disc 2. The packaged production source remains `f4ddc36469b9490cce20b293c553b2a080b6dfbf`. A test that reads the original field script takes it from the player’s local game data.

## Apply the patches

Use a clean private engine checkout at `af746eead91bbe5725fe2d4c415f51dbe7a7ea9d`. Set `patch_dir` to the absolute path of this directory. These commands verify the complete file tree after each patch with the starting checkout’s private documentation.

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
git apply --unidiff-zero --index --check "$patch_dir/forest-save-follow-camera.patch"
git apply --unidiff-zero --index "$patch_dir/forest-save-follow-camera.patch"
test "$(git write-tree)" = 355d50fa90f34633cc334455e94615a169b54d79
git apply --unidiff-zero --index --check "$patch_dir/forest-fixture-maintenance.patch"
git apply --unidiff-zero --index "$patch_dir/forest-fixture-maintenance.patch"
test "$(git write-tree)" = 3b9ac739926d0eb1f2503c02ef5e6e7b0c707a5b
```

The source and tests are complete after the sequence above. Engineering commit `5559cdc3b9298c3181966049c3119dbef3675572` updated `HANDOVER.md`. Engineering commit `aa0b9d25766bc5c8070d21cfd9466f3a80c17942` also updated `docs/port/NATIVE_CONVERSION.md` while changing the test runner. To reproduce the recorded engineering tree including those private documents, run these additional commands in a checkout that has both commits.

```sh
git restore --source=5559cdc3b9298c3181966049c3119dbef3675572 --staged --worktree -- HANDOVER.md
git restore --source=aa0b9d25766bc5c8070d21cfd9466f3a80c17942 --staged --worktree -- docs/port/NATIVE_CONVERSION.md
test "$(git write-tree)" = 2014d702f1e8686ad986e4f06ce75c23bdc920fd
```

| Patch | SHA-256 | Private engineering commit |
| --- | --- | --- |
| forest-resume.patch | `9e2fb182b7af46a9ca0e661a1f04c820b890c9c7c1925877b11e16029d863d8e` | `35c01e13c31bebe99c4280dff1c98f7206940351` |
| forest-resume-tests.patch | `9b0a5effc100273fc78dcd36c13e1ff0bbd896b2f3e7e129b51511d60ef3f2b6` | `370e10b9d6cde39f7362012f7a792fe5129d6cfe` |
| forest-ordered-handback.patch | `1bb20827a5ef027c3595c59d9d27b1302572d9630f413f33e9edf902c45a7cc8` | `4235be4dce40f62f67570869ea636ad2b39f2162` |
| forest-save-follow-camera.patch | `923ad6ce3f3590a579b155be3286c267eb2bf325841647d2c0c3424db3b7398c` | `f4ddc36469b9490cce20b293c553b2a080b6dfbf` |
| forest-fixture-maintenance.patch | `28ef09d96dc7247c5fb43892f69b32cc0762ca008fcdaca3e01a7c1cf8ea9a8b` | `da6618e27b5c7ae664564574eba49b264d557379` |

## Install the private repository guard

Before pushing from this standalone private topic checkout, run the following command from its repository root. It installs the repository’s existing retail-file guard into that checkout.

```sh
sh scripts/install-retail-guard.sh
```

## Forest movement and workshop renderer update

A sixth patch preserves Fei’s first walking step after loading a newly saved Disc 2 forest checkpoint. It also changes the native renderer’s triangle coverage in the workshop. Apply it after the five patches above from the starting checkout, before restoring either private document.

```sh
git apply --unidiff-zero --index --check "$patch_dir/forest-stride-workshop-renderer.patch"
git apply --unidiff-zero --index "$patch_dir/forest-stride-workshop-renderer.patch"
test "$(git write-tree)" = 65953771adb40ff5aeb310597d9c5e971829ef07
```

The patch has SHA-256 `1d7b7dd36787c6b28d71314100df5ad393881e2a29bb4ad17b32610973ca4680`. The private engineering build used commit `8531ff633e4153b89363822238445c62cec692e0`. A checkout containing that engineering commit can reproduce its two handover documents and verify the complete tree.

```sh
git restore --source=8531ff633e4153b89363822238445c62cec692e0 --staged --worktree -- HANDOVER.md docs/port/NATIVE_CONVERSION.md
test "$(git write-tree)" = ae109520dce74e1805866637b8359eeec8783988
```

## Returning to Taura's workshop

The seventh patch preserves Elly's facing and the party trail in a new save at the pre-meeting workshop entrance. It also corrects draw order inside an opaque textured quad. Apply it after the sixth patch, with the starting documentation still in place.

```sh
git apply --unidiff-zero --index --check "$patch_dir/taura-workshop-save-and-quad.patch"
git apply --unidiff-zero --index "$patch_dir/taura-workshop-save-and-quad.patch"
test "$(git write-tree)" = 6ccc8dfa2c42aa9e86b861bf09255e5f2cf0e4ce
```

The patch SHA-256 is `a2cdcc382e69d06a232ca5fef410c9db8727953101237b3ee4e2236a38de6c5b`. The Linux engineering source is `2d7acef268cf8679df5c477e28d18e766c18ab49`. An engineering checkout with that commit can restore its private handover documents and verify its tree.

```sh
git restore --source=2d7acef268cf8679df5c477e28d18e766c18ab49 --staged --worktree -- HANDOVER.md docs/port/NATIVE_CONVERSION.md
test "$(git write-tree)" = 3d8dcc1eee4a67a1871753952e75f7ef0b70c48c
```

In a clean Windows engineering checkout at `7a79bc5499c11684154255b84c03664b1f2ac61f`, applying this seventh patch by itself produces tree `7616acbb0468ed24ec87623c98dd145c7bea5a0c`, the complete source at `703cb99a8c2c3cd14026bd2fcccadb1ac8917ac5`.
