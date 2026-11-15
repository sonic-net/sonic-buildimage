#!/usr/bin/env bash
# Shared CI entry point for canonical and explicitly pinned registry builds.
set -euo pipefail

# base_review.py copies this file alone into a temporary directory. Only the
# source-image entry point uses the adjacent filtered-rc generator.
source_review=false
backend=bazel
revision=${SONIC_BASE_REVIEW_REGISTRY:-}
if [[ ${1:-} == --source-image-review ]]; then
  shift
  source_review=true
  backend=${BAZEL_REAL:-bazel}
  revision=${SONIC_BAZEL_REGISTRY_REVISION:-}
fi

if [[ $# == 1 && "$1" == --version ]]; then
  if "$source_review"; then
    # The resource launcher prepends startup options, so use a real command.
    # Preserve subsequent output, stderr, and the backend's exit status.
    "$backend" version --gnu_format | sed '1{/^Bazelisk /d;}'
    exit
  fi
  exec bazel --version
fi

registry=https://raw.githubusercontent.com/securely1g/sonic-bazel-registry/main
if "$source_review" || [[ -n "$revision" ]]; then
  if [[ ! "$revision" =~ ^[0-9a-f]{40}$ ]]; then
    echo 'Expected a full lowercase registry commit for review.' >&2
    exit 2
  fi
  registry="${registry%/main}/$revision"
fi

startup_options=()
while [[ ${1:-} == --* ]]; do
  case "$1" in
    --batch|--host_jvm_args=*|--output_base=*|--output_user_root=*)
      startup_options+=("$1"); shift ;;
    *)
      if ! "$source_review"; then break; fi
      case "$1" in
        --bazelrc|--bazelrc=*|--*system_rc|--*system_rc=*|--*home_rc|--*home_rc=*|--*workspace_rc|--*workspace_rc=*|--ignore_all_rc_files*)
          echo 'Review builds select their own rc files.' >&2; exit 2 ;;
        --output_base|--output_user_root|--host_jvm_args|--max_idle_secs|--io_nice_level|--connect_timeout_secs|--server_javabase|--install_base)
          if [[ $# -lt 2 ]]; then printf 'Missing value for %s\n' "$1" >&2; exit 2; fi
          startup_options+=("$1" "$2"); shift 2 ;;
        *) startup_options+=("$1"); shift ;;
      esac ;;
  esac
done
bazel_command=${1:-}
case "$bazel_command" in
  aquery|build|cquery|fetch|info|mod|query|test) shift ;;
  *) printf 'Unsupported CI Bazel command: %s\n' "$bazel_command" >&2; exit 2 ;;
esac

arguments=()
if [[ -n "$revision" ]]; then
  while [[ $# -gt 0 && "$1" != -- ]]; do
    case "$1" in
      --registry|--registry=*) echo 'Duplicate review registry selection.' >&2; exit 2 ;;
      *) arguments+=("$1") ;;
    esac
    shift
  done
  # Put the enforced policy after the caller's options but before a possible
  # "--" target separator. query and mod do not execute build actions.
  case "$bazel_command" in
    aquery|build|cquery|fetch|info|test) arguments+=(--remote_upload_local_results=false) ;;
  esac
fi
arguments+=("$@")

rc="$PWD/tools/bazel/common.bazelrc"
if "$source_review"; then
  rc=$(mktemp "${TMPDIR:-/tmp}/sonic-review-bazel.XXXXXXXX")
  trap 'rm -f -- "$rc"' EXIT
  python3 "$(dirname "$0")/review_bazel_rc.py" "$rc"
fi
command=("$backend" "${startup_options[@]}" --nosystem_rc --nohome_rc --noworkspace_rc
  --bazelrc="$rc" "$bazel_command" --registry="$registry"
  --registry=https://bcr.bazel.build --lockfile_mode=update "${arguments[@]}")
if "$source_review"; then
  "${command[@]}"
else
  exec "${command[@]}"
fi
