#!/bin/bash
set -euo pipefail

HOST_MACHINE="/host/machine.conf"
DEVICE_INFO_DIR="/etc/device"
SUBVERSION_FILE="${DEVICE_INFO_DIR}/.subversion"
FIRSTBOOT_MARKER="/tmp/notify_firstboot_to_platform"

should_refresh_device_info() {
    if [ -f "$FIRSTBOOT_MARKER" ]; then
        return 0
    fi

    for path in \
        "${DEVICE_INFO_DIR}/.productname" \
        "${DEVICE_INFO_DIR}/productname" \
        "${DEVICE_INFO_DIR}/.board_id" \
        "${DEVICE_INFO_DIR}/board_id"; do
        if [ ! -f "$path" ]; then
            return 0
        fi
    done

    return 1
}

get_machine_var() {
    local key="$1"
    awk -F= -v key="$key" '$1 == key {print $2; exit}' "$HOST_MACHINE"
}

ensure_onie_board_id() {
    local onie_platform="$1"
    local onie_board_id

    onie_board_id="$(get_machine_var "onie_board_id" || true)"
    if [ -n "$onie_board_id" ]; then
        return 0
    fi

    case "$onie_platform" in
    arm64-micas_m2-w6950-128oc-r0)
        printf '%s\n' 'onie_board_id=0x414e' >> "$HOST_MACHINE"
        ;;
    esac
}

write_file() {
    local name="$1"
    local value="$2"
    local path="${DEVICE_INFO_DIR}/${name}"

    printf '%s\n' "$value" > "${path}.tmp"
    mv -f "${path}.tmp" "$path"
}

main() {
    if ! should_refresh_device_info; then
        exit 0
    fi

    if [ ! -f "$HOST_MACHINE" ]; then
        echo "machine.conf not found at $HOST_MACHINE" >&2
        exit 1
    fi

    local onie_platform
    local onie_board_id
    local onie_subversion
    local default_sku
    local device_dir

    onie_platform="$(get_machine_var "onie_platform")"

    if [ -z "$onie_platform" ]; then
        echo "onie_platform is empty in $HOST_MACHINE" >&2
        exit 1
    fi

    ensure_onie_board_id "$onie_platform"

    onie_board_id="$(get_machine_var "onie_board_id")"
    onie_subversion="$(get_machine_var "onie_subversion" || true)"

    if [ -z "$onie_board_id" ]; then
        echo "onie_board_id is empty in $HOST_MACHINE" >&2
        exit 1
    fi

    mkdir -p "$DEVICE_INFO_DIR"
    device_dir="/usr/share/sonic/device/${onie_platform}"
    default_sku=""
    if [ -f "${device_dir}/default_sku" ]; then
        default_sku="$(tr -d '\n' < "${device_dir}/default_sku")"
    fi
    if [ -z "$default_sku" ]; then
        default_sku="$(printf '%s' "$onie_platform" | sed -e 's/^arm64-micas_//' -e 's/-r[0-9][0-9]*$//' | tr '[:lower:]' '[:upper:]')"
    fi

    write_file ".productname" "$(printf '%s' "$onie_platform" | tr '-' '_')"
    write_file "productname" "$default_sku"
    write_file ".board_id" "${onie_board_id,,}"
    write_file "board_id" "${onie_board_id,,}"

    if [ -n "$onie_subversion" ]; then
        printf '%s\n' "$onie_subversion" > "${SUBVERSION_FILE}.tmp"
        mv -f "${SUBVERSION_FILE}.tmp" "$SUBVERSION_FILE"
    else
        rm -f "$SUBVERSION_FILE"
    fi
}

main "$@"
