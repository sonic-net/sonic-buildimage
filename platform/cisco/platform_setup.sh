#!/bin/bash
# Copyright 2015-2026 Cisco Systems, Inc.
# SPDX-License-Identifier: BSD-3-Clause
# See platform/cisco/COPYRIGHT for the full license text.

set -e

ROOT=$(pwd)
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)

if [ -f "${SCRIPT_DIR}/src/rules.mk" ]; then
    bash "${SCRIPT_DIR}/src/platform_setup.sh"
    PLATFORM_CONF_SOURCE="${SCRIPT_DIR}/src/platform.conf"
else
    bash "${SCRIPT_DIR}/embedded/platform_setup.sh"
    PLATFORM_CONF_SOURCE="${SCRIPT_DIR}/embedded/platform.conf"
fi

if [ ! -f "${PLATFORM_CONF_SOURCE}" ]; then
    echo "Missing Cisco platform configuration: ${PLATFORM_CONF_SOURCE}" >&2
    exit 1
fi

# build_image.sh expects this path. It is ignored and points at the selected
# source-backed or embedded platform configuration.
PLATFORM_CONF_LINK="${SCRIPT_DIR}/platform.conf"
rm -f "${PLATFORM_CONF_LINK}"
PLATFORM_CONF_TARGET=$(realpath --relative-to="${SCRIPT_DIR}" "${PLATFORM_CONF_SOURCE}")
ln -s "${PLATFORM_CONF_TARGET}" "${PLATFORM_CONF_LINK}"
