#!/usr/bin/env bash

set -euo pipefail

# shellcheck source=../frr_config_security.sh
. "$(dirname "$0")/../frr_config_security.sh"

test_dir=$(mktemp -d "${TMPDIR:-/tmp}/frr-config-files.XXXXXX")
trap 'rm -rf -- "$test_dir"' EXIT

touch "${test_dir}/target" "${test_dir}/regular.conf"
ln "${test_dir}/target" "${test_dir}/zebra.conf"
ln -s "${test_dir}/target" "${test_dir}/bgpd.conf"
ln -s "${test_dir}/target" "${test_dir}/sharpd.conf"
ln -s "${test_dir}/target" "${test_dir}/unrelated.conf"

prepare_frr_config_files "$test_dir" bgpd.conf sharpd.conf zebra.conf regular.conf

[ ! -e "${test_dir}/bgpd.conf" ]
[ ! -e "${test_dir}/sharpd.conf" ]
[ ! -e "${test_dir}/zebra.conf" ]
[ -f "${test_dir}/regular.conf" ]
[ -L "${test_dir}/unrelated.conf" ]
[ -f "${test_dir}/target" ]

mkdir "${test_dir}/frr.conf"
if prepare_frr_config_files "$test_dir" frr.conf 2>/dev/null; then
    echo 'Non-file configuration path was accepted' >&2
    exit 1
fi
