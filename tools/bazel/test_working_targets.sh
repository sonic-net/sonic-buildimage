#!/bin/bash
set -euo pipefail

repo_root=$(git rev-parse --show-toplevel)

function test_repo() {
  repo=$1
  cmd=$2
  echo "[test] ${repo}: ${cmd}"
  pushd "${repo_root}/${repo}"
  ${cmd}
  popd
}

echo "[= Testing Dependent Repositories =]"

test_repo "src/sonic-build-infra" "bazel build ..."

echo "[= Testing Docker Images =]"

cd "${repo_root}"
bazel query 'kind(oci_load, ...)'

for load in $(bazel query 'kind(oci_load, ...)'); do
    echo "[load] ${load}"
    bazel run "${load}"
done

echo "[= DONE =]"
