#!/usr/bin/env bash
set -euo pipefail

workspace=$PWD
kernel=src/sonic-linux-kernel
case "${1:-}" in
    ''|--bazel-only|--kernel-only|--populated-only) ;;
    *) echo "Usage: $0 [--bazel-only|--kernel-only|--populated-only]" >&2; exit 2 ;;
esac

# Root Bazel module resolution reads each local_path_override, even when a job
# builds just one package. Artifact validators only need their own sources.
if [[ -z ${1:-} || ${1:-} == --bazel-only ]]; then
    mapfile -t paths < "$workspace/tools/bazel/submodule_paths"
    if ((${#paths[@]})); then
        git -c safe.directory="$workspace" -C "$workspace" submodule sync -- "${paths[@]}"
        git -c safe.directory="$workspace" -C "$workspace" submodule update --init --checkout --depth 1 -- "${paths[@]}"
    fi
    if [[ ${1:-} == --bazel-only ]]; then
        exit 0
    fi
fi

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

# Package CI also needs gNOI. Artifact consumers initialize only the owner
# required by their own service after restoring the populated worktrees.
if [[ "${1:-}" != --populated-only ]]; then
    git -c safe.directory="$workspace" submodule update --init --checkout -- src/sonic-sysmgr/gnoi
fi
