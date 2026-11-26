# Disc 2 forest checkpoint

A cold load of an earned checkpoint after the workshop dialogue resumes the forest encounter's handback and restores the party and follow camera at the saved pose. On a fresh visit the dialogue runs as usual. The accompanying test covers checkpoint identity, script readiness, resumed control and rejection of inconsistent inputs. Its optional positive case reads the player's own field script.

Use an isolated, clean checkout of private engineering revision `af746eead91bbe5725fe2d4c415f51dbe7a7ea9d`. Set `patch_dir` to the absolute path of this directory, then run the following commands from the engine checkout.

```sh
test "$(git rev-parse HEAD)" = af746eead91bbe5725fe2d4c415f51dbe7a7ea9d
test -z "$(git status --porcelain)"
git apply --unidiff-zero --index --check "$patch_dir/forest-resume.patch"
git apply --unidiff-zero --index "$patch_dir/forest-resume.patch"
test "$(git write-tree)" = 2302b9c338a7452046c6a80f8d559e63a3f48b21
git apply --unidiff-zero --index --check "$patch_dir/forest-resume-tests.patch"
git apply --unidiff-zero --index "$patch_dir/forest-resume-tests.patch"
test "$(git write-tree)" = dfd67676891d9b6daaea3e69518cb1a02a416cc1
```

| Patch | SHA-256 | Corresponding private engineering revision |
| --- | --- | --- |
| forest-resume.patch | `9e2fb182b7af46a9ca0e661a1f04c820b890c9c7c1925877b11e16029d863d8e` | `35c01e13c31bebe99c4280dff1c98f7206940351` |
| forest-resume-tests.patch | `9b0a5effc100273fc78dcd36c13e1ff0bbd896b2f3e7e129b51511d60ef3f2b6` | `370e10b9d6cde39f7362012f7a792fe5129d6cfe` |

The private engine checkout supplies the remaining source and build system.
