#!/bin/bash
# Compares every Bazel-built artifact (.deb archives and container images) against its Make-built counterpart.
set -Eeuo pipefail

trap 'echo "[FAILED] ${BASH_SOURCE[0]}:${LINENO}: ${BASH_COMMAND}" >&2' ERR

repo_root=$(git rev-parse --show-toplevel)
cd "${repo_root}"

BLDENV="${BLDENV:-trixie}"

# rcache, because we don't want this job writing to the shared cache.
# See rules/config for the modes.
CACHE_OPTIONS="${CACHE_OPTIONS:-SONIC_DPKG_CACHE_METHOD=rcache}"

function run_in_slave() {
  if [[ "${SKIP_SLAVE:-0}" == "1" ]]; then
    eval "$1"
    return
  fi
  make -f Makefile.work "BLDENV=${BLDENV}" sonic-slave-run \
    SONIC_RUN_CMDS="cd /sonic && $1"
}

# Ensure the tree is clean before proceeding.
if [[ -n "$(git status --porcelain)" ]]; then
  if [[ "${EQUIVALENCE_ALLOW_DIRTY:-0}" != "1" ]]; then
    echo "ERROR: the checkout is dirty. Both sides must come from the same tree." >&2
    git status --short >&2
    echo "Set EQUIVALENCE_ALLOW_DIRTY=1 to compare anyway." >&2
    exit 1
  fi
  echo "WARNING: comparing on a dirty tree. Results may be inaccurate."
fi

# Invoked directly rather than through `bazel run`, because the script shells out to Bazel itself.
compare="PYTHONPATH=tools/bazel/registry python3 tools/bazel/equivalence_checker/equivalence_checker.py --bldenv ${BLDENV}"

# elfcompare shells out to abidiff for shared libraries,
# and we haven't migrated abidiff to Bazel yet.
#
# We use .dockerenv to figure out whether we're in the slave.
# If we're not in the slave, we shouldn't be installing anything.
#
# TODO(bazel-ready): fetch abidiff the way src/protobuf fetches protoc.
# There is no abigail module in the BCR, but Debian has one in the snapshot we
# already pin, and abidiff needs nothing exotic to run:
#
#     abigail-tools 2.6-2, NEEDED libabigail.so.5 libstdc++.so.6 libgcc_s.so.1 libc.so.6
#
# So the two `.deb`s (abigail-tools and libabigail5) plus a LD_LIBRARY_PATH
# wrapper would do it. The package's python3-git and python3-libarchive-c
# dependencies belong to abipkgdiff and abidb, not to abidiff, so they can stay
# out of it.
provision_abidiff=$(tr '\n' ' ' <<'EOF'
if ! command -v abidiff >/dev/null; then
  if [ -f /.dockerenv ]; then
    sudo apt-get update &&
      sudo apt-get install -y --no-install-recommends abigail-tools;
  else
    echo ERROR: abidiff is not installed. Install abigail-tools to run this comparison. >&2;
    exit 1;
  fi;
fi
EOF
)

# Assert that we're not trying to build with Bazel.
# Otherwise, we'd be comparing Bazel to itself.
if [[ "${BUILD_WITH_BAZEL_WHEN_AVAILABLE:-n}" != "n" ]]; then
  echo "ERROR: BUILD_WITH_BAZEL_WHEN_AVAILABLE must be disabled, otherwise we'll be comparing Bazel to itself." >&2
  exit 1
fi

echo "[= Finding the Make artifacts to compare against =]"
make_artifacts_file="target/.equivalence-artifacts"
rm -f "${make_artifacts_file}"
# abidiff is provisioned even here, because the comparison resolves every tool it
# might need up front, before it works out what there is to compare.
run_in_slave "${provision_abidiff} && ${compare} --print-make-paths > ${make_artifacts_file}"

# Make the non-debug targets before the debug ones, to avoid trampling each other's cache.
mapfile -t make_artifacts < <(
  grep -v -- "-dbg" "${make_artifacts_file}"
  grep -- "-dbg" "${make_artifacts_file}"
)

if [[ ${#make_artifacts[@]} -eq 0 ]]; then
  echo "ERROR: found no Make artifacts to compare against." >&2
  exit 1
fi

echo "[= Building the Make side =]"
printf '[make] %s\n' "${make_artifacts[@]}"
env ${CACHE_OPTIONS} "BLDENV=${BLDENV}" make "${make_artifacts[@]}"

echo "[= Comparing =]"
run_in_slave "${provision_abidiff} && ${compare}"
