#!/bin/sh
# Install the pre-push retail guard into this clone.
set -e
root=$(git rev-parse --show-toplevel)
cp "$root/scripts/retail-guard-pre-push.sh" "$root/.git/hooks/pre-push"
chmod +x "$root/.git/hooks/pre-push"
echo "retail guard installed as .git/hooks/pre-push"
