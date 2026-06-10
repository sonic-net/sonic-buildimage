#!/bin/bash
set -e

# Mock directory
MOCK_DIR=$(mktemp -d)
trap 'rm -rf "$MOCK_DIR"' EXIT

# Mock standard tools
cp /bin/echo "$MOCK_DIR/echo"
cp /bin/cat "$MOCK_DIR/cat"
cp /bin/grep "$MOCK_DIR/grep"
cp /bin/sed "$MOCK_DIR/sed"
cp /usr/bin/awk "$MOCK_DIR/awk"
cp /usr/bin/find "$MOCK_DIR/find"

# Export MOCK_DIR for the script
export MOCK_DIR
export PATH="$MOCK_DIR:$PATH"

# Source default_platform.conf
# We need to mock some sourcing or just source it directly if we are in the right dir
# We will copy default_platform.conf to MOCK_DIR and source it
INSTALLER_ROOT=$(pwd)
if [ ! -f "default_platform.conf" ]; then
    INSTALLER_ROOT=$(dirname $(dirname $(realpath $0)))
fi
cp "${INSTALLER_ROOT}/default_platform.conf" "$MOCK_DIR/"
cp "${INSTALLER_ROOT}/install.sh" "$MOCK_DIR/" 2>/dev/null || true

# Mock absolute paths in default_platform.conf
sed -i "s|/host|${MOCK_DIR}/host|g" "$MOCK_DIR/default_platform.conf"
sed -i "s|/boot/efi|${MOCK_DIR}/boot/efi|g" "$MOCK_DIR/default_platform.conf"
sed -i "s|/efi|${MOCK_DIR}/efi|g" "$MOCK_DIR/default_platform.conf"

# Mock functions expected by default_platform.conf
trap_push() { :; }
is_mounted() { return 0; }
demo_install_uefi_grub() {
    echo "FAIL: demo_install_uefi_grub called (should be skipped)"
    exit 1
}
demo_install_grub() {
    echo "FAIL: demo_install_grub called (should be skipped)"
    exit 1
}
demo_install_uefi_shim() {
    echo "FAIL: demo_install_uefi_shim called (should be skipped)"
    exit 1
}

# Mock efivar to return nothing or error to avoid secure boot path if wanted
efivar() { return 1; }

# Mock variables used in bootloader_menu_config
export install_env="sonic"
export firmware="uefi"
export demo_mnt="$MOCK_DIR/mnt"
export demo_volume_label="SONiC-OS"
export demo_volume_revision_label="SONiC-OS-new-version"
export image_dir="image-new-version"
export blk_dev="/dev/mockblk"
export arch="amd64"
export demo_type="OS"
export ONIE_PLATFORM_EXTRA_CMDLINE_LINUX=""
export kargs=""
export platform="x86_64-kvm_x86_64"
export bootloader_state_machine="$MOCK_DIR/bootloader_state_machine.sh"
touch "$bootloader_state_machine"

# Setup Mock Filesystem
mkdir -p "$demo_mnt"
mkdir -p "$MOCK_DIR/esp/EFI/SONiC-OS"

# Helper to run test
run_test() {
    local test_name="$1"
    local running_slot="$2" # A or B
    local existing_cfg_content="$3"

    echo "Running Test: $test_name"

    # Reset grub config
    # Detection looks at $demo_mnt/grub/grub.cfg
    # In test, demo_mnt is $MOCK_DIR/mnt
    mkdir -p "$demo_mnt/grub"
    local esp_cfg="$demo_mnt/grub/grub.cfg"
    echo "$existing_cfg_content" > "$esp_cfg"

    if [ "$running_slot" == "A" ]; then
        export running_sonic_revision="vA"
    elif [ "$running_slot" == "B" ]; then
        export running_sonic_revision="vB"
    else
        export running_sonic_revision="unknown"
    fi

    # Source and run
    (
        . "$MOCK_DIR/default_platform.conf"
        bootloader_menu_config
    )

    # Verify results
    if [ "$running_slot" == "A" ]; then
        # New image should replace B
        # The new body contains "linux   /image-new-version/boot/vmlinuz..."
        if grep -q "menuentry \"SONIC_B\" {" "$esp_cfg" && grep -q "linux   /image-new-version/boot/vmlinuz" "$esp_cfg"; then
            echo "PASS: Updated SONIC_B"
        else
            echo "FAIL: Did not update SONIC_B correctly"
            cat "$esp_cfg"
            exit 1
        fi
        # A should be preserved (pointing to vA)
        if grep -q "menuentry \"SONIC_A\" {" "$esp_cfg" && grep -q "image-vA" "$esp_cfg"; then
             echo "PASS: Preserved SONIC_A"
        else
             echo "FAIL: SONIC_A invalid"
             cat "$esp_cfg"
             exit 1
        fi
    fi

    if [ "$running_slot" == "B" ]; then
        # New image should replace A
        if grep -q "menuentry \"SONIC_A\" {" "$esp_cfg" && grep -q "linux   /image-new-version/boot/vmlinuz" "$esp_cfg"; then
            echo "PASS: Updated SONIC_A"
        else
            echo "FAIL: Did not update SONIC_A correctly"
            echo "--- Config Content ---"
            cat "$esp_cfg"
            echo "----------------------"
            exit 1
        fi
        # B should be preserved (pointing to vB)
        if grep -q "menuentry \"SONIC_B\" {" "$esp_cfg" && grep -q "image-vB" "$esp_cfg"; then
             echo "PASS: Preserved SONIC_B"
        else
             echo "FAIL: SONIC_B invalid"
             cat "$esp_cfg"
             exit 1
        fi
    fi

    # Verify garbage preservation
    if grep -q "SONIC_GARBAGE" "$esp_cfg"; then
        echo "PASS: Garbage entry preserved (as expected by current code)"
    else
        echo "FAIL: Garbage entry REMOVED (unexpected)"
        exit 1
    fi
}

# content for grub.cfg
cfg_content='
menuentry "SONIC_A" {
    linux /image-vA/boot/vmlinuz root=UUID=...
}
menuentry "SONIC_GARBAGE" {
    linux /image-garbage/boot/vmlinuz
}
menuentry "SONIC_B" {
    linux /image-vB/boot/vmlinuz root=UUID=...
}
menuentry "SONIE" {
    chainloader /linux.efi
}
'

# Run Tests
run_test "Replace B when A is running" "A" "$cfg_content"
run_test "Replace A when B is running" "B" "$cfg_content"
