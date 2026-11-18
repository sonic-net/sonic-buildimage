#!/usr/bin/env bash
set -euo pipefail

workspace=${GITHUB_WORKSPACE:-$PWD}
kernel=src/sonic-linux-kernel
case "${1:-}" in
    ''|--kernel-only) ;;
    *) echo "Usage: $0 [--kernel-only]" >&2; exit 2 ;;
esac

# A nonrecursive checkout can leave a previous job's kernel at its old commit.
# Jobs that only check source cleanliness do not need to clone the kernel.
if [[ -e "$workspace/$kernel/.git" ]]; then
    parent=(git -c safe.directory="$workspace" -c safe.directory="$workspace/$kernel" -C "$workspace")
    child=(git -c safe.directory="$workspace/$kernel" -C "$workspace/$kernel")
    "${parent[@]}" diff --cached --exit-code HEAD -- "$kernel"
    expected=$("${parent[@]}" rev-parse "HEAD:$kernel")
    actual=$("${child[@]}" rev-parse HEAD)
    if [[ "$actual" != "$expected" ]]; then
        changes=$("${child[@]}" status --porcelain --untracked-files=normal --ignore-submodules=none)
        if [[ -n "$changes" ]]; then
            printf 'Cannot update modified %s from %s to %s\n%s\n' "$kernel" "$actual" "$expected" "$changes" >&2
            exit 1
        fi
        "${parent[@]}" submodule update --init --checkout -- "$kernel"
    fi
fi
if [[ "${1:-}" == --kernel-only ]]; then
    exit 0
fi

# Restore populated worktrees, including nested ones left by earlier jobs.
# A populated worktree can lose its local registration when a checkout is reused.
git -c safe.directory="$workspace" submodule foreach --recursive \
    'git -c safe.directory="$toplevel" -c safe.directory="$toplevel/$sm_path" --literal-pathspecs -C "$toplevel" submodule update --init --checkout -- "$sm_path"'

# Only gNOI is required on a fresh runner.
git -c safe.directory="$workspace" submodule update --init --checkout -- src/sonic-sysmgr/gnoi
