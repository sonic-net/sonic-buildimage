#!/bin/bash
# Copyright 2015-2026 Cisco Systems, Inc.
# SPDX-License-Identifier: BSD-3-Clause
# See platform/cisco/COPYRIGHT for the full license text.

# Embedded setup for PLATFORM=cisco when the vendor checkout is unavailable.
# Stage the common data and platform directories below device/cisco. The
# sonic-device-data package flattens this staging layout, preserving the
# runtime common path as /usr/share/sonic/device/cisco-8000.
set -e

ROOT=$(pwd)
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
DEVICE_SOURCE=${SCRIPT_DIR}/device

remove_staged_symlinks() {
    local path=$1

    if [ -L "${path}" ]; then
        unlink "${path}"
        return
    fi

    if [ -d "${path}" ]; then
        while IFS= read -r -d '' link; do
            unlink "${link}"
        done < <(find -P "${path}" -type l -print0)
    fi
}

mkdir -p "${ROOT}/device"

# Remove links left by either source-backed setup or an earlier embedded run.
# Do not follow or remove their targets. Existing directories are retained so
# their nested staging links can be refreshed below.
remove_staged_symlinks "${ROOT}/device/cisco"
remove_staged_symlinks "${ROOT}/device/cisco-8000"

mkdir -p "${ROOT}/device/cisco"

# Keep this list aligned with CISCO_PLATFORM_HW_SKUS in platform-rules.mk.
# Do not stage new vendor platforms just because their directories exist.
for platform_name in cisco-8000 \
    x86_64-8101_32fh_o-r0 \
    x86_64-8101_32fh_o_c01-r0 \
    x86_64-8102_64h_o-r0; do
    platform_source="${DEVICE_SOURCE}/${platform_name}"
    if [ ! -d "${platform_source}" ]; then
        echo "ERROR: missing embedded device data: ${platform_source}" >&2
        exit 1
    fi

    platform_link="${ROOT}/device/cisco/${platform_name}"
    if [ -e "${platform_link}" ]; then
        echo "ERROR: ${platform_link} exists and is not a symlink" >&2
        exit 1
    fi
    platform_target=$(realpath --relative-to="${ROOT}/device/cisco" "${platform_source}")
    ln -s "${platform_target}" "${platform_link}"
done
