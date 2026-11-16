# Source inside one build's shell. Related build, query and export commands use
# the same server. Independent recipes always have different writable state.
start_private_bazel() {
    local owner=${1##*/}
    shift
    local parent
    parent=$(realpath -- "${TMPDIR:-/tmp}") || return
    local base
    base=$(mktemp -d -- "$parent/sonic-bazel.$owner.XXXXXXXX") || return
    open_private_bazel "$base" "$@"
}

open_private_bazel() {
    bazel_output_base=$1
    shift
    private_bazel_keep_output=0
    private_bazel_workspace=$PWD
    private_bazel=("${BAZEL:-bazel}" "$@" "--output_base=$bazel_output_base")
    private_bazel_shutdown_prefix=()
    trap 'finish_private_bazel "$?"' EXIT
    trap 'exit 129' HUP
    trap 'exit 130' INT
    trap 'exit 143' TERM
}

# /sonic is the job checkout in both disposable Trixie containers. Preserve
# these two producers' on-disk outputs for the separate VS validation step.
start_image_bazel() {
    local owner=$1
    if [[ ${SONIC_VS_REUSE_BAZEL_PRODUCERS:-} == 1 &&
          ( $owner == docker-orchagent.gz || $owner == docker-syncd-vs.gz ) ]]; then
        local base="$PWD/target/.vs-bazel/$owner"
        mkdir -p -- "$base" || return
        open_private_bazel "$base"
        private_bazel_keep_output=1
    else
        start_private_bazel "$owner"
    fi
}

# Each new output base belongs to one workspace. After the build, read its
# generated execroot directly; "bazel info" can misresolve Bzlmod platform labels.
private_bazel_execution_root() {
    local roots=("$bazel_output_base"/execroot/*/bazel-out)
    if (( ${#roots[@]} != 1 )) || [[ ! -d "${roots[0]}" ]]; then
        printf 'Expected one Bazel execution root in %s/execroot\n' "$bazel_output_base" >&2
        return 1
    fi
    dirname -- "${roots[0]}"
}

finish_private_bazel() {
    local status=$1 cleanup_status=0
    cd -- "$private_bazel_workspace" &&
        "${private_bazel_shutdown_prefix[@]}" "${private_bazel[@]}" shutdown || cleanup_status=$?
    if (( cleanup_status == 0 && private_bazel_keep_output == 0 )); then
        # Some Bazel actions leave read-only directories inside the output tree.
        # Writable directory entries suffice to unlink its files, including cache hardlinks.
        if ! rm -rf -- "$bazel_output_base" 2>/dev/null; then
            find "$bazel_output_base" -type d ! -perm -700 -exec chmod u+rwx -- {} \; &&
                rm -rf -- "$bazel_output_base" || cleanup_status=$?
        fi
    fi
    if (( cleanup_status )); then
        printf 'Could not clean up private Bazel server at %s\n' "$bazel_output_base" >&2
    fi
    if (( status )); then exit "$status"; fi
    exit "$cleanup_status"
}
