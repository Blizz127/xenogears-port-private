#!/bin/sh
# pre-push retail guard: refuse to push any commit that adds game-data files.
# Installed by scripts/install-retail-guard.sh as .git/hooks/pre-push.
set -e
pattern='\.(bin|cue|iso|chd|img|nes|rom|sav|mcd|mcr|ecm|pbp)$|(^|/)extracted/'
z40=0000000000000000000000000000000000000000
while read local_ref local_sha remote_ref remote_sha; do
  [ "$local_sha" = "$z40" ] && continue
  if [ "$remote_sha" = "$z40" ]; then range="$local_sha"; else range="$remote_sha..$local_sha"; fi
  bad=$(git diff --name-only --diff-filter=A "$remote_sha" "$local_sha" 2>/dev/null || git ls-tree -r --name-only "$local_sha") 
  hits=$(printf '%s\n' "$bad" | grep -E -i "$pattern" || true)
  if [ -n "$hits" ]; then
    echo "retail guard: refusing to push game-data files:" >&2
    printf '%s\n' "$hits" >&2
    exit 1
  fi
done
exit 0
