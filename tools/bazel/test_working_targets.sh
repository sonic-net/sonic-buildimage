#!/bin/bash
set -euo pipefail

repo_root=$(git rev-parse --show-toplevel)

function run_in_slave() {
  repo=$1
  cmd=$2

  # Run the same command inside the sonic-slave-bookworm container.
  if [[ "${SKIP_SLAVE:-0}" != "1" ]]; then
    echo "[slave] ${repo}: ${cmd}"
    make -C "${repo_root}" -f Makefile.work BLDENV=bookworm sonic-slave-run \
      SONIC_RUN_CMDS="cd /sonic/${repo} && ${cmd}"
  fi
}

function test_repo() {
  repo=$1
  cmd=$2

  echo "[host] ${repo}: ${cmd}"
  pushd "${repo_root}/${repo}"
  ${cmd}
  bazel clean
  popd

  run_in_slave $1 $2
}

echo "[= Testing Dependent Repositories =]"

test_repo "src/sonic-build-infra" "bazel build ..."
test_repo "src/sonic-swss-common" "bazel build ..."
test_repo "src/sonic-sysmgr" "bazel build ..."
test_repo "src/libnl3" "bazel build ..."
test_repo "src/libyang3" "bazel build ..."

echo "[= Testing Docker Images =]"

cd "${repo_root}"
set +e
docker_images=$(
  bazel query --keep_going --output=package 'kind(oci_image, ...)' | sed 's:^dockers/::' 2>/dev/null
)
set -e

for image in ${docker_images[@]}; do
    echo "[docker-make] ${image}"

    make "target/${image}.gz"
done

echo "[= DONE =]"
