# Disc 2 forest checkpoint

This change restores the forest encounter controller and recenters the follow camera when the player loads an earned checkpoint after the workshop dialogue. The disc script runs the party handback. The port checks the checkpoint, actors and disc-script identity before resuming it.

Apply the patch in an isolated checkout of the private engine. The required base commit is `af746eead91bbe5725fe2d4c415f51dbe7a7ea9d`. In the following commands, set `patch_file` to the absolute path of this directory's `forest-resume.patch`.

```sh
test "$(git rev-parse HEAD)" = af746eead91bbe5725fe2d4c415f51dbe7a7ea9d
test -z "$(git status --porcelain)"
git apply --unidiff-zero --index --check "$patch_file"
git apply --unidiff-zero --index "$patch_file"
test "$(git write-tree)" = 2302b9c338a7452046c6a80f8d559e63a3f48b21
```

The patch SHA-256 is `9e2fb182b7af46a9ca0e661a1f04c820b890c9c7c1925877b11e16029d863d8e`. The resulting file tree matches private engineering revision `35c01e13c31bebe99c4280dff1c98f7206940351`. The existing private engine supplies the build system and all other source files.
