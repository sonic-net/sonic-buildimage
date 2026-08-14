#!/bin/bash
#
# stage-submodule-patches.sh
#
# Apply Ciena-maintained quilt patches that target a git SUBMODULE ROOT which is
# built as individual sub-packages (so the stock per-package quilt rule in
# slave.mk never applies a submodule-root src/<name>.patch).
#
# Example: sonic-platform-daemons is one submodule that builds many wheels
# (sonic-xcvrd, sonic-psud, ...). Each wheel's SRC_PATH is a sub-directory, e.g.
# src/sonic-platform-daemons/sonic-xcvrd, so slave.mk looks for the patch at
# src/sonic-platform-daemons/sonic-xcvrd.patch (inside the submodule, which the
# parent repo cannot track). Unlike sonic-utilities (a top-level submodule that
# IS a package SRC_PATH, so src/sonic-utilities.patch is applied by the stock
# rule), there is no package whose SRC_PATH == src/sonic-platform-daemons, so the
# top-level src/sonic-platform-daemons.patch would never be applied.
#
# This hook applies those top-level patches to the submodule working tree after
# `git submodule update`, exactly like src/sonic-utilities.patch (same layout:
# src/<name>.patch/{series,*.patch}; paths relative to the submodule root). It is
# invoked from the Makefile.work `init` and `reset` targets and is idempotent.

set -eu

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

# Submodules whose root patch (src/<name>.patch) must be applied by this hook
# because they have no top-level build package to trigger the stock quilt apply.
SUBMODULE_ROOT_PATCHES="sonic-platform-daemons"

for name in $SUBMODULE_ROOT_PATCHES; do
    patchdir="src/${name}.patch"
    submodule="src/${name}"
    series="${patchdir}/series"

    [ -f "$series" ] || continue
    if [ ! -d "$submodule" ]; then
        echo "stage-submodule-patches: ${submodule} not checked out yet; skipping"
        continue
    fi

    while IFS= read -r p || [ -n "$p" ]; do
        case "$p" in ''|\#*) continue ;; esac
        pf="${REPO_ROOT}/${patchdir}/${p}"
        [ -f "$pf" ] || { echo "stage-submodule-patches: missing $pf"; exit 1; }

        # Idempotent: skip if the patch already applies in reverse (already in).
        if git -C "$submodule" apply -p1 --reverse --check "$pf" >/dev/null 2>&1; then
            echo "stage-submodule-patches: already applied ${name}/${p}"
        else
            git -C "$submodule" apply -p1 "$pf"
            echo "stage-submodule-patches: applied ${name}/${p} to ${submodule}"
        fi
    done < "$series"
done
