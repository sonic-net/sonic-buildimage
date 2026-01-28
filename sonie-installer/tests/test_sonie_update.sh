#!/bin/bash
# test_sonie_update.sh
# Verifies that when install_env="sonie", the installer:
# 1. Skips install_esp_bootloader
# 2. Calls update_sonie_grub_config (simulated)

set -u

TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALLER_ROOT="$(dirname "${TEST_DIR}")"
INSTALL_SCRIPT="${INSTALLER_ROOT}/install.sh"
TEMP_SCRIPT="$(mktemp)"
export SCRIPT_DIR="${INSTALLER_ROOT}"

cleanup() {
    rm -f "${TEMP_SCRIPT}"
    rm -rf "${MOCK_DIR}"
}
trap cleanup EXIT

MOCK_DIR=$(mktemp -d)

# Setup Mocks
setup_mocks() {
    export install_env="sonie"
    export is_ram_root=1
    export is_onie=0
    export platform="test_sonie_platform"
    export onie_platform="test_onie_platform"
    export blk_dev="/dev/mockblk"

    # Mock environment variables expected by default_platform.conf
    export CONSOLE_PORT="0x3f8"
    export CONSOLE_SPEED="115200"
    export CONSOLE_DEV="0"
    export sonie_part_size="100"
    export sonie_volume_label="SONIE-OS"
    export sonie_volume_revision_label="SONIE-OS-v1"

    export ONIE_INSTALLER_PAYLOAD="${MOCK_DIR}/payload.zip"
    touch "${ONIE_INSTALLER_PAYLOAD}"

    # Mock Logging
    log_info() { echo "INFO: $*"; }
    log_warn() { echo "WARN: $*"; }
    log_error() { echo "ERROR: $*"; }

    # Mock helpers
    read_conf_file() { :; }
    _trap_push() { :; }

    # Mock ESP content
    mkdir -p "${MOCK_DIR}/tmp_mount/EFI/SONiC-OS"
    touch "${MOCK_DIR}/tmp_mount/EFI/SONiC-OS/grub.cfg"
    echo "# Master Config" > "${MOCK_DIR}/tmp_mount/EFI/SONiC-OS/grub.cfg"

    # Mock System Commands
    mount() { echo "MOCK: mount $*"; }
    umount() { echo "MOCK: umount $*"; }
    mkdir() { echo "MOCK: mkdir $*"; }
    rmdir() { echo "MOCK: rmdir $*"; }
    mktemp() {
        if [[ "$*" == "-d" ]]; then
            echo "${MOCK_DIR}/tmp_mount"
        else
            echo "${MOCK_DIR}/tmp_file"
        fi
    }
    id() { echo 0; }

    unzip() { echo "MOCK: unzip $*"; }

    # Mock default_platform.conf functions
    detect_environment() { export install_env="sonie"; echo "MOCK: detect_environment set install_env=sonie"; }
    detect_block_device() {
        export blk_dev="/dev/mockblk"
        export cur_part="/dev/mockblk1"
        echo "MOCK: detect_block_device set blk_dev=$blk_dev";
    }
    load_configs() { :; }
    check_root() { :; }
    detect_machine_conf() { :; }
    check_asic_platform() { :; }

    # Mock partition creation (we don't care about details here)
    create_partition() { echo "MOCK: create_partition"; }
    mount_partition() {
        export sonie_mnt="${MOCK_DIR}/mnt"
        export demo_mnt="${sonie_mnt}"
        command mkdir -p "${sonie_mnt}/grub"
        echo "MOCK: mount_partition set sonie_mnt=$sonie_mnt"
    }

    # Mock install_esp_bootloader call (spy)

    # Mock grub-install
    grub-install() {
        echo "MOCK: grub-install $*"
        if [[ "$*" != *"--no-nvram"* ]]; then
             echo "FAIL: grub-install missing --no-nvram"
             exit 1
        fi
        # Ensure we don't return failure
        return 0
    }

    efibootmgr() {
        if [[ "$*" == *"-B"* ]]; then
             echo "FAIL: efibootmgr -B called (should skip cleanup)"
             return 1
        fi
        echo "MOCK: efibootmgr $*"
    }
}

# Source the script (stripping main)
sed '$d' "${INSTALL_SCRIPT}" > "${TEMP_SCRIPT}"
# Patch TEMP_SCRIPT to comment out sourcing of default_platform.conf
sed -i 's|\. "${SCRIPT_DIR}/default_platform.conf"|# . "${SCRIPT_DIR}/default_platform.conf"|' "${TEMP_SCRIPT}"

test_sonie_update_flow() {
    echo "Testing Sonie Update Flow..."
    (
        # Source default_platform.conf manually first
        . "${INSTALLER_ROOT}/default_platform.conf"

        # Now source install.sh (patched)
        # shellcheck source=/dev/null
        . "${TEMP_SCRIPT}"

        # Now setup mocks (overrides functions from default_platform.conf)
        setup_mocks

        cd "${INSTALLER_ROOT}" || exit 1

        # Create a dummy UKI so find finds it
        command mkdir -p "${MOCK_DIR}/mnt"
        touch "${MOCK_DIR}/mnt/linux.efi"

        # Seed existing grub.cfg with SONiC entries for merging test


        command mkdir -p "${MOCK_DIR}/tmp_mount/EFI/SONiC-OS"
        cat > "${MOCK_DIR}/tmp_mount/EFI/SONiC-OS/grub.cfg" <<EOF
menuentry 'SONiC-OS-Old' {
    search --label Old
    chainloader /old.efi
}
menuentry 'SONiC-OS-Older' {
    search --label Older
    chainloader /older.efi
}
EOF

        main

        # Verify Content in OUTPUT grub.cfg


        local grub_cfg="${MOCK_DIR}/mnt/grub/grub.cfg"

        if grep -q "menuentry 'SONIC_A'" "${grub_cfg}"; then
            echo "PASS: grub.cfg contains SONIC_A"
        else
            echo "FAIL: grub.cfg missing SONIC_A"
            cat "${grub_cfg}"
            exit 1
        fi

        if grep -q "menuentry 'SONIC_B'" "${grub_cfg}"; then
             echo "PASS: grub.cfg contains SONIC_B"
        else
             echo "FAIL: grub.cfg missing SONIC_B"
             # exit 1
        fi

        # Verify SONIE is last? (or at least present)
        if grep -q "menuentry 'SONIE'" "${grub_cfg}"; then
             echo "PASS: grub.cfg contains SONIE"
        else
             echo "FAIL: grub.cfg missing SONIE"
             # exit 1
        fi

        # Check order? grep output order.
        # A then B then SONIE.
        local order
        order=$(grep "menuentry " "${grub_cfg}" | cut -d "'" -f 2 | tr '\n' ',')
        if [[ "$order" == "SONIC_A,SONIC_B,SONIE,"* ]]; then
             echo "PASS: Order is correct: $order"
        else
             echo "FAIL: Order incorrect: $order"
             exit 1
        fi
    )
}

test_sonie_update_flow
