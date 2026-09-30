#!/bin/sh
# Install the pre-push retail guard into this clone (works in linked worktrees too).
set -e
root=$(git rev-parse --show-toplevel)
hooks=$(git rev-parse --git-path hooks)
case "$hooks" in /*) ;; *) hooks="$(git rev-parse --show-toplevel)/$hooks" ;; esac
mkdir -p "$hooks"
cp "$root/scripts/retail-guard-pre-push.sh" "$hooks/pre-push"
chmod +x "$hooks/pre-push"
echo "retail guard installed as $hooks/pre-push"
