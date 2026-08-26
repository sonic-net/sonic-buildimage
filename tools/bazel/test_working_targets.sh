#!/bin/bash
set -Eeuo pipefail

# This script is long and mostly quiet until something breaks, 
# so name the command that failed.
trap 'echo "[FAILED] ${BASH_SOURCE[0]}:${LINENO}: ${BASH_COMMAND}" >&2' ERR

repo_root=$(git rev-parse --show-toplevel)

cmd_root="${repo_root}"
if [[ "${SKIP_SLAVE:-0}" != "1" ]]; then
  cmd_root=/sonic
fi

function run_in_slave() {
  local repo=$1
  local cmd=$2

  # SKIP_SLAVE=1 still runs the command, just on the host. Skipping it outright
  # would make the whole script exit 0 while testing nothing.
  if [[ "${SKIP_SLAVE:-0}" == "1" ]]; then
    echo "[host] ${repo}: ${cmd}"
    (cd "${repo_root}/${repo}" && eval "${cmd}")
    return
  fi

  # Run the same command inside the sonic-slave-trixie container.
  echo "[slave] ${repo}: ${cmd}"
  make -C "${repo_root}" -f Makefile.work BLDENV=trixie sonic-slave-run \
    SONIC_RUN_CMDS="cd /sonic/${repo} && ${cmd}"
}

function test_repo() {
  local repo=$1

  echo "[test_repo] ${repo}"
  # TODO(bazel-ready): Formalize and standardize these checks when we have a better idea of what we need.
  run_in_slave "${repo}" "bazel clean"
  run_in_slave "${repo}" "bazel build ..."
  # Bazel exits with '4' if there are no tests, and we want to accept that as a success instead.
  run_in_slave "${repo}" "${cmd_root}/tools/bazel/bazel_test_allow_empty.sh ..."
  run_in_slave "${repo}" "bazel run //tools/bazel/buildifier:buildifier.check"
}

echo "[= Testing Docker Images =]"

cd "${repo_root}"

# --keep_going: not every package in the repo loads, and we only want the ones
# declaring an oci_image, so a non-zero exit is expected here.
docker_images=$(
  bazel query --keep_going --output=package 'kind(oci_image, ...)' 2>/dev/null | sed 's:^dockers/::'
) || true

if [[ -z "${docker_images}" ]]; then
  echo "ERROR: found no oci_image packages" >&2
  exit 1
fi
echo "[== Found at least one Docker Image ==]"

for image in ${docker_images}; do
    # Every Bazel-built container has a paired debug image, and both go through
    # the same sonic_docker_archive chain, so build both.
    for archive in "${image}.gz" "${image}-dbg.gz"; do
      echo "[docker-make] ${archive}"

      rm -f "target/${archive}"
      BUILD_WITH_BAZEL_WHEN_AVAILABLE=true \
        BLDENV=trixie \
        make "target/${archive}"
    done
done

echo "[= Testing sonic-buildimage =]"

run_in_slave "." "bazel test ..."

echo "[= Testing Dependent Repositories =]"

test_repo "src/sonic-build-infra"
test_repo "src/sonic-swss-common"
test_repo "src/sonic-sysmgr"
test_repo "src/libnl3"

echo "[= DONE =]"
