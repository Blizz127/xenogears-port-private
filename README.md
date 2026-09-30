# xenogears-port-private

Private port development. Never commit game data (discs, ROMs, BIOS, extracted assets).
Public releases happen only at owner-chosen milestones.

- Checkpoint builds are published here as PRIVATE releases (tarball + SHA256SUMS) and mirrored
  into the owner's private launcher channel by `publish-private-port-build`.
- Retail guard: before your first push, run `scripts/install-retail-guard.sh` in your clone.
  It installs a pre-push hook that refuses any commit adding game-data files.
