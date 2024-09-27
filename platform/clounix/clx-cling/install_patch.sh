#!/bin/bash

CLOUNIX_SAI_SRC="$1"
CLOUNIC_SDK_SRC="${CLOUNIX_SAI_SRC}/src/sdk"
CLOUNIX_CLING_PATCH="$(dirname $(readlink -f "$0"))/patch"
CLX_CLING_VERSION="1.0.0"

if [ ! -f "${CLOUNIC_SDK_SRC}/app/diag_shell/cmd/clx_cling_cmd.c" ]; then
    patch -d "${CLOUNIC_SDK_SRC}" -p1 -i "${CLOUNIX_CLING_PATCH}/clx_cling_cmd.patch" >/dev/null
fi
echo "$CLX_CLING_VERSION"
