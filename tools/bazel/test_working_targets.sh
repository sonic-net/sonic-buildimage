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

echo "[= Testing Local Tests =]"

bazel test \
  //dockers/docker-orchagent/tests:site-packages_assert \
  //dockers/docker-base-bookworm/tests:site-packages_assert \
  @libyang3_py3//... \
  @libyang//... \
  --keep_going \
  --test_output=errors

echo "[= Testing Dependent Repositories =]"

test_repo "src/sonic-build-infra" "bazel build ..."
test_repo "src/sonic-utilities" "bazel build :sonic-utilities :dist"
test_repo "src/sonic-utilities" "bazel test //:all --test_output=errors"
test_repo "src/sonic-host-services" "bazel build :sonic-host-services :dist"
test_repo "src/sonic-host-services" "bazel test //:all --test_output=errors"
test_repo "src/sonic-sairedis/SAI" "bazel build ..."
test_repo "src/sonic-sairedis" "bazel build ..."
test_repo "src/sonic-dash-api" "bazel build ..."
test_repo "src/sonic-swss-common" "bazel build ..."
test_repo "src/sonic-swss" "bazel build ..."
test_repo "src/sonic-p4rt/sonic-pins" "bazel build ..."
test_repo "src/sonic-mgmt-common" "bazel build ..."
test_repo "src/sonic-gnmi" "bazel build ..."

echo "[= Testing Docker Images =]"

cd "${repo_root}"
bazel query 'kind(oci_load, ...)'

# for load in $(bazel query 'kind(oci_load, ...)'); do
for load in $(bazel query 'kind(oci_load, ...) - //dockers/docker-sonic-p4rt:load'); do
    echo "[load] ${load}"
    bazel run "${load}"
done

echo "[= DONE =]"
