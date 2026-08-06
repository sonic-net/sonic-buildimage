#!/bin/bash
set -euo pipefail

repo_root=$(git rev-parse --show-toplevel)

function run_in_slave() {
  repo=$1
  cmd=$2

  # Run the same command inside the sonic-slave-trixie container.
  if [[ "${SKIP_SLAVE:-0}" != "1" ]]; then
    echo "[slave] ${repo}: ${cmd}"
    make -C "${repo_root}" -f Makefile.work BLDENV=trixie sonic-slave-run \
      SONIC_RUN_CMDS="cd /sonic/${repo} && ${cmd}"
  fi
}

function test_repo() {
  repo=$1

  echo "[test_repo] ${repo}"
  # TODO(bazel-ready): Formalize and standardize these checks when we have a better idea of what we need.
  run_in_slave "${repo}" "bazel clean"
  run_in_slave "${repo}" "bazel build ..."
  # Relax bash requirements, because Bazel exits with '4' if there are no tests.
  set +euo pipefail
  run_in_slave "${repo}" "bazel test ... || [[ $? == 4 ]]"
  set -eup pipefail
  run_in_slave "${repo}" "bazel run //tools/bazel/buildifier:buildifier.check"
}

echo "[= Testing sonic-buildimage =]"

run_in_slave "." "bazel test ..."

echo "[= Testing Dependent Repositories =]"

test_repo "src/sonic-build-infra"
test_repo "src/sonic-swss-common"
test_repo "src/sonic-sysmgr"
test_repo "src/libnl3"

echo "[= Testing Docker Images =]"

cd "${repo_root}"
set +e
docker_images=$(
  bazel query --keep_going --output=package 'kind(oci_image, ...)' | sed 's:^dockers/::' 2>/dev/null
)
set -e

for image in ${docker_images[@]}; do
    echo "[docker-make] ${image}"

    rm -f "target/${image}.gz"
    BUILD_WITH_BAZEL_WHEN_AVAILABLE=true \
      BLDENV=trixie \
      make "target/${image}.gz"
done

echo "[= DONE =]"
