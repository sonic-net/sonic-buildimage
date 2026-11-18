#!/usr/bin/env bash
set -euo pipefail

# Restore populated worktrees, including nested ones left by earlier jobs.
# A populated worktree can lose its local registration when a checkout is reused.
workspace=${GITHUB_WORKSPACE:-$PWD}
git -c safe.directory="$workspace" submodule foreach --recursive \
    'git -c safe.directory="$toplevel" -c safe.directory="$toplevel/$sm_path" --literal-pathspecs -C "$toplevel" submodule update --init --checkout -- "$sm_path"'

# Only gNOI is required on a fresh runner.
git -c safe.directory="$workspace" submodule update --init --checkout -- src/sonic-sysmgr/gnoi
