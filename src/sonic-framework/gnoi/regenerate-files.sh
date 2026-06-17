#!/bin/bash
set -euo pipefail

BASE=$(bazel info bazel-genfiles)
GNOI_NS='github.com/openconfig/gnoi'

copy_generated() {
  pkg="$1"
  # Default to using package name for proto if $2 is unset
  proto="$1" && [ "${2++}" ] && proto="$2"
  # Bazel go_rules will create empty files containing "// +build ignore\n\npackage ignore"
  # in the case where the protoc compiler doesn't generate any output. See:
  # https://github.com/bazelbuild/rules_go/blob/03a8b8e90eebe699d7/go/tools/builders/protoc.go#L190
  for file in "${BASE}"/"${pkg}"/"${proto}"_go_proto_/"${GNOI_NS}"/"${pkg}"/*.pb.go; do
    [[ $(head -n 1 "${file}") == "// +build ignore" ]] || cp "${file}" "${pkg}"/
  done
}

bazel build //bgp:all
copy_generated "bgp"
bazel build //bootconfig:all
copy_generated "bootconfig"
bazel build //cert:all
copy_generated "cert"
bazel build //common:all
copy_generated "common"
bazel build //containerz:all
copy_generated "containerz"
bazel build //debug:all
copy_generated "debug"
bazel build //diag:all
copy_generated "diag"
bazel build //factory_reset:all
copy_generated "factory_reset"
bazel build //file:all
copy_generated "file"
bazel build //healthz:all
copy_generated "healthz"
bazel build //layer2:all
copy_generated "layer2"
bazel build //mpls:all
copy_generated "mpls"   
bazel build //os:all
copy_generated "os"
bazel build //otdr:all
copy_generated "otdr"
bazel build //packet_capture:all
copy_generated "packet_capture" "pcap"
bazel build //packet_link_qualification:all
copy_generated "packet_link_qualification" "linkqual"
bazel build //system:all
copy_generated "system"
bazel build //types:all
copy_generated "types"
bazel build //wavelength_router:all
copy_generated "wavelength_router"
