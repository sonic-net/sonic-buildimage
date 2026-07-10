#!/bin/bash
#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Copy selected Mellanox vendor custom SAI headers into sonic-sairedis/SAI/custom/
# before OCP saimetadata.c is generated for libsaimetadata.so.
#
# Only headers required for the libsaimetadata / libsai bind-point enum alignment
# are synced (saitypescustom.h merges SAI_ACL_BIND_POINT_TYPE_CPU_PORT). Full vendor
# custom headers are intentionally omitted until parse.pl can version them cleanly.

set -euo pipefail

usage()
{
    echo "Usage: $0 <mlnx-sai.deb|mlnx-sai.tar.gz> <sonic-sairedis/SAI/custom/>" >&2
    exit 1
}

if [ "$#" -ne 2 ]; then
    usage
fi

MLNX_SAI_PKG="$1"
CUSTOM_DIR="$2"
SAI_META_DIR="$(dirname "$CUSTOM_DIR")/meta"

if [ ! -e "$MLNX_SAI_PKG" ]; then
    echo "ERROR: mlnx-sai package not found: $MLNX_SAI_PKG" >&2
    exit 1
fi

if [ ! -d "$CUSTOM_DIR" ]; then
    echo "ERROR: custom header directory not found: $CUSTOM_DIR" >&2
    exit 1
fi

TMPDIR="$(mktemp -d)"
cleanup()
{
    rm -rf "$TMPDIR"
}
trap cleanup EXIT

if [[ "$MLNX_SAI_PKG" == *.deb ]]; then
    dpkg-deb -x "$MLNX_SAI_PKG" "$TMPDIR"
    HEADER_SRC="$TMPDIR/usr/include/sai"
elif [[ "$MLNX_SAI_PKG" == *.tar.gz ]]; then
    tar -xzf "$MLNX_SAI_PKG" -C "$TMPDIR"
    if [ -d "$TMPDIR/mlnx_sai/inc/custom" ]; then
        HEADER_SRC="$TMPDIR/mlnx_sai/inc/custom"
    elif [ -d "$TMPDIR/inc/custom" ]; then
        HEADER_SRC="$TMPDIR/inc/custom"
    else
        echo "ERROR: could not locate custom headers in tarball: $MLNX_SAI_PKG" >&2
        exit 1
    fi
else
    echo "ERROR: unsupported mlnx-sai package type: $MLNX_SAI_PKG" >&2
    exit 1
fi

# Bind-point fix only: merge sai_acl_bind_point_type_custom_t (CPU_PORT) into OCP metadata.
MLNX_SAI_METADATA_HEADERS=(
    saitypescustom.h
)

headers=()
missing=()
for header in "${MLNX_SAI_METADATA_HEADERS[@]}"; do
    if [ -f "$HEADER_SRC/$header" ]; then
        headers+=( "$HEADER_SRC/$header" )
    else
        missing+=( "$header" )
    fi
done

if [ "${#missing[@]}" -ne 0 ]; then
    echo "ERROR: required custom SAI headers not found under $HEADER_SRC: ${missing[*]}" >&2
    exit 1
fi

# Only now that all required headers are confirmed present, drop stale vendor
# headers from a previous sync (keep upstream README) and install the new set.
# Deleting earlier would leave $CUSTOM_DIR empty if validation above failed.
find "$CUSTOM_DIR" -maxdepth 1 -type f -name '*custom*.h' -delete
cp -f "${headers[@]}" "$CUSTOM_DIR"/

# parse.pl always emits "#include <saicustom.h>" when custom/ is non-empty. Vendor
# saicustom.h pulls in every *custom*.h header, which OCP parse.pl cannot version
# cleanly; write a minimal umbrella instead. The custom #include lines are generated
# from MLNX_SAI_METADATA_HEADERS so the umbrella can never drift from the curated set
# that is actually synced above (add a header to that array and it is included here).
# Include at least one @flags free enum so Doxygen/parse.pl get a valid sectiondef.
{
    cat <<'EOF'
#ifndef __SAICUSTOM_H_
#define __SAICUSTOM_H_

#include <sai.h>
#include <saitypes.h>
EOF
    for header in "${MLNX_SAI_METADATA_HEADERS[@]}"; do
        echo "#include \"$header\""
    done
    cat <<'EOF'

/**
 * @brief Custom SAI APIs
 *
 * @flags free
 */
typedef enum _sai_api_custom_t {
    SAI_API_CUSTOM_RANGE_START = SAI_API_CUSTOM_RANGE_BASE,

    /* Add new custom APIs above this line */

    SAI_API_CUSTOM_RANGE_END
} sai_api_custom_t;

#endif /* __SAICUSTOM_H_ */
EOF
} > "$CUSTOM_DIR/saicustom.h"

echo "Synced ${#headers[@]} Mellanox custom SAI header(s) into $CUSTOM_DIR:"
for header in "${MLNX_SAI_METADATA_HEADERS[@]}"; do
    echo "  $CUSTOM_DIR/$header"
done
echo "  $CUSTOM_DIR/saicustom.h (generated minimal umbrella)"

# Force saimetadata regeneration when custom headers change.
rm -f \
    "$SAI_META_DIR/saimetadata.c" \
    "$SAI_META_DIR/saimetadata.h" \
    "$SAI_META_DIR/saimetadatasize.h" \
    "$SAI_META_DIR/saimetadatatest.c"
