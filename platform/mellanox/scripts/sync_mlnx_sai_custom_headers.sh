#!/bin/bash
#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Copy Mellanox vendor custom SAI headers into sonic-sairedis/SAI/custom/
# before OCP saimetadata.c is generated for libsaimetadata.so.
#
# Sync the complete vendor custom-header set selected by saicustom.h. Custom object
# types and APIs must be kept together so libsaimetadata matches the selected SAI.

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

# Treat the vendor umbrella as the authoritative custom-header manifest. This
# keeps custom object types (for example RBB_CLASSIFIER), their API declarations,
# and custom enums (for example CPU_PORT) from being synced independently.
if [ ! -f "$HEADER_SRC/saicustom.h" ]; then
    echo "ERROR: required custom SAI umbrella not found: $HEADER_SRC/saicustom.h" >&2
    exit 1
fi

mapfile -t included_headers < <(
    sed -nE 's/^[[:space:]]*#include[[:space:]]*"([^"]+)".*/\1/p' \
        "$HEADER_SRC/saicustom.h"
)

headers=( "$HEADER_SRC/saicustom.h" )
missing=()
for header in "${included_headers[@]}"; do
    if [ -f "$HEADER_SRC/$header" ]; then
        headers+=( "$HEADER_SRC/$header" )
    else
        missing+=( "$header" )
    fi
done

if [ "${#missing[@]}" -ne 0 ]; then
    echo "ERROR: custom SAI headers included by saicustom.h not found under $HEADER_SRC: ${missing[*]}" >&2
    exit 1
fi

# Only after the complete set is validated, replace headers from a previous sync
# while preserving the upstream README.
find "$CUSTOM_DIR" -maxdepth 1 -type f -name '*.h' -delete
cp -f "${headers[@]}" "$CUSTOM_DIR"/

echo "Synced ${#headers[@]} Mellanox custom SAI header(s) into $CUSTOM_DIR:"
for header in "${headers[@]}"; do
    echo "  $CUSTOM_DIR/$(basename "$header")"
done

# parse.pl expands custom ACL META_DATA_GROUP MIN..MAX ranges into synthetic _1.._N
# attributes. Those are not literal enums, so attrversion.sh never versions them and
# WriteMetaDataFiles treats the resulting warnings as fatal. Alias them to MIN the
# same way upstream already does for USER_DEFINED_FIELD_GROUP_*.
PARSE_PL="$SAI_META_DIR/parse.pl"
if [ -f "$PARSE_PL" ] && ! grep -q 'SAI_ACL_ENTRY_ATTR_FIELD_ACL_USER_META_DATA_GROUP_MIN' "$PARSE_PL"; then
    python3 - "$PARSE_PL" <<'PY'
import pathlib, sys
path = pathlib.Path(sys.argv[1])
text = path.read_text()
needle = '''    $attr = "SAI_ACL_TABLE_ATTR_USER_DEFINED_FIELD_GROUP_MIN"
        if ($attr =~ /^SAI_ACL_TABLE_ATTR_USER_DEFINED_FIELD_GROUP_\\d+$/);

    if (not defined $ATTR_API_VER{$attr} and scalar(keys%ATTR_API_VER) != 0)'''
insert = '''    $attr = "SAI_ACL_TABLE_ATTR_USER_DEFINED_FIELD_GROUP_MIN"
        if ($attr =~ /^SAI_ACL_TABLE_ATTR_USER_DEFINED_FIELD_GROUP_\\d+$/);

    # Mellanox custom ACL user-meta group ranges (saiaclcustom.h). parse.pl expands
    # MIN..MAX into _1.._N; those synthetic values are not literals for attrversion.sh.
    $attr = "SAI_ACL_ENTRY_ATTR_FIELD_ACL_USER_META_DATA_GROUP_MIN"
        if ($attr =~ /^SAI_ACL_ENTRY_ATTR_FIELD_ACL_USER_META_DATA_GROUP_\\d+$/);

    $attr = "SAI_ACL_TABLE_ATTR_FIELD_ACL_USER_META_DATA_GROUP_MIN"
        if ($attr =~ /^SAI_ACL_TABLE_ATTR_FIELD_ACL_USER_META_DATA_GROUP_\\d+$/);

    $attr = "SAI_ACL_ENTRY_ATTR_ACTION_SET_ACL_META_DATA_GROUP_MIN"
        if ($attr =~ /^SAI_ACL_ENTRY_ATTR_ACTION_SET_ACL_META_DATA_GROUP_\\d+$/);

    $attr = "SAI_ACL_ENTRY_ATTR_ACTION_SET_ACL_META_DATA_MASK_GROUP_MIN"
        if ($attr =~ /^SAI_ACL_ENTRY_ATTR_ACTION_SET_ACL_META_DATA_MASK_GROUP_\\d+$/);

    if (not defined $ATTR_API_VER{$attr} and scalar(keys%ATTR_API_VER) != 0)'''
if needle not in text:
    raise SystemExit(f"ERROR: unexpected parse.pl ProcessApiVersion format: {path}")
path.write_text(text.replace(needle, insert, 1))
print(f"Patched {path} for custom ACL META_DATA_GROUP version aliases")
PY
fi

# Force saimetadata regeneration when custom headers change.
rm -f \
    "$SAI_META_DIR/saimetadata.c" \
    "$SAI_META_DIR/saimetadata.h" \
    "$SAI_META_DIR/saimetadatasize.h" \
    "$SAI_META_DIR/saimetadatatest.c"
