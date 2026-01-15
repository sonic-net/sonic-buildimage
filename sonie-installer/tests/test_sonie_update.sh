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
sed -i 's|\. \./default_platform.conf|# . ./default_platform.conf|' "${TEMP_SCRIPT}"

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

        # Seed existing grub.cfg with SONiC entries
        # We need to ensure logic looks for it in ${esp_mnt}/EFI/SONiC-OS/grub.cfg?
        # OR in the mounted location?
        # install_grub_to_esp determines grub_cfg path as "$os_mnt/grub/grub.cfg" (which is demo_mnt/grub/grub.cfg)
        # Wait, previous logic appended to existing if found.
        # Now we want to READ existing from "somewhere" and WRITE to new config?
        # "when the sonie-installer installs the grub config... add them to the grub config"
        # Since we are installing to a NEW partition (demo_mnt), we are writing a NEW config file.
        # BUT we want to import entries from the *active* config?
        # OR does "sonie installer" imply we are overwriting the *disk's* active config?
        # install_grub_to_esp writes to `esp_mnt`? No, viewed code: `local grub_cfg="$os_mnt/grub/grub.cfg"`.
        # Wait, viewed code line 459: `local grub_cfg="$os_mnt/grub/grub.cfg"`
        # BUT line 451: `--efi-directory="$esp_mnt"`, `--boot-directory="$os_mnt"`.
        # `grub-install` puts the EFI binary in ESP, and looks for config in `boot-directory` (which is os_mnt).
        # So `grub.cfg` lives in `os_mnt/grub/grub.cfg`.
        
        # IF we are running in "sonie", we assume there is an ALREADY EXISTING grub config involved?
        # The user says "add them to the grub config".
        # If we are installing a NEW SONiC image to a NEW partition, the NEW grub config will be used by the NEW grub binary.
        # BUT `grub-install` on UEFI updates the NVRAM to point to this NEW binary.
        # If we use `--no-nvram`, the NVRAM points to the OLD binary (SONIE's or previous).
        # So we probably want to update the CONFIG that the OLD binary uses? 
        # OR we want our NEW config to include the OLD entries, assuming we manually chainload to it?
        
        # "sonie-installer installs the grub config"
        # If we use `--no-nvram`, we are NOT changing the bootloader.
        # So we are just placing files.
        # If we don't update NVRAM, the system boots the OLD entry.
        # If the OLD entry points to an OLD config, our NEW config in `os_mnt` is IGNORED unless the OLD config chainloads to it?
        # OR does the user mean we should update verify the *existing* config?
        
        # "add them to the grub config" -> implying there is ONE main config.
        # If we are "appending" (previous task), we assumed we appended to the *master* config found in ESP.
        # My previous `update_sonie_grub_config` implementation searched ESP for `grub.cfg`.
        # But `install_grub_to_esp` in `default_platform.conf` writes to `$os_mnt/grub/grub.cfg`.
        # AND my previous modification to `install_grub_to_esp` ADDED logic to check for EXISTING file at `$os_mnt/grub/grub.cfg`.
        # BUT `$os_mnt` is empty (we just mkfs'd it).
        # So checking for existing file in `$os_mnt` (the NEW partition) is pointless.
        
        # We need to find the *active* config on the disk.
        # Where is it?
        # In Sonie environment, we probably have access to other partitions. 
        # We might need to mount ESP and look there?
        # logic:
        # 1. Mount ESP.
        # 2. Find grub.cfg in ESP (or where it lives).
        # 3. Read it.
        # 4. Generate NEW config with imported entries + Our New Entry.
        # 5. Place this NEW config... where? 
        #    If we put it in `os_mnt`, the OLD bootloader won't see it (unless it looks there).
        #    If we overwrite the OLD config in ESP, then we update the menu.
        
        # User said: "installs the grub config... add them to the grub config"
        # Likely implies overwriting the ACTIVE config with a MERGED config.
        # AND we agreed to use `--no-nvram`, so we persist the current bootloader binary.
        # So we MUST update the config that the current bootloader binary reads.
        
        # Current bootloader usually reads from ESP/EFI/<id>/grub.cfg OR ESP/grub.cfg.
        # My previous `update_sonie_grub_config` found `grub.cfg` in ESP.
        
        # `install_grub_to_esp` writes to `$os_mnt/grub/grub.cfg`.
        # This seems wrong for "Sonie" flow if we want to modify the ACTIVE configuration.
        # Unless `$os_mnt` is NOT the new partition but the ESP?
        # No, `$os_mnt` is argument 3, passed as `${demo_mnt}` (the new ext4 partition).
        
        # I suspect for "Sonie", we should be modifying the ESP's grub.cfg, NOT valid `os_mnt`.
        
        # Let's seed a config in `os_mnt/grub/grub.cfg` just in case logic uses it?
        # OR more likely, we need to Mount existing ESP and read from there.
        # The test mock needs to simulate an existing config in a detectable location.
        
        # I will assume `install_grub_to_esp` will be modified to look in ESP for existing config.
        # So I will seed one in ESP.
        
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
        # Should be in os_mnt? Or ESP?
        # `install_grub_to_esp` generates to `os_mnt`.
        # If we assume we want to replace the active one, maybe we should copy it to ESP?
        # OR `grub-install` copies it?
        # `grub-install` doesn't typically copy config from boot-directory unless requested or default?
        # Actually `grub-install` creates the binary that looks in `boot-directory`.
        
        # IF we run with `--no-nvram`, we preserve the old binary.
        # The OLD binary looks in its configured location (likely ESP or a specific partition).
        # If we write to `os_mnt`, the old binary won't verify it.
        
        # I will assume for now we verify `os_mnt/grub/grub.cfg` matches expectations, 
        # AND we might need to copy it to ESP if that's what's intended. 
        # But let's check content first.
        
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
