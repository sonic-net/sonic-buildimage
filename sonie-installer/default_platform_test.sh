#!/bin/bash

# shellcheck disable=SC2329,SC1090,SC2034

# Test harness for default_platform.conf

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

DEFAULT_PLATFORM_CONF="${SCRIPT_DIR}/default_platform.conf"

# Mocks
mock_files=()

# Mock for cat command
cat() {
  # If the file exists in our mock list, return the mock content
  if [[ "${1}" == "/proc/cmdline" ]]; then
    echo "${MOCK_CMDLINE}"
  elif [[ "${1}" == "/proc/mounts" ]]; then
    echo "${MOCK_MOUNTS}"
  elif [[ "${1}" == "/proc/cpuinfo" ]]; then
    echo "${MOCK_CPUINFO}"
  else
    # Fallback to real cat for other files
    /bin/cat "$@"
  fi
}

# Mock for grep command
grep() {
  /bin/grep "$@"
}

# Mock for cut command
cut() {
  /bin/cut "$@"
}

# Mock for blkid command
blkid() {
  if [[ "$1" == "-o" ]] && [[ "$2" == "export" ]]; then
    echo "${MOCK_BLKID_EXPORT:-}"
  else
    echo "${MOCK_BLKID}"
  fi
}

# Mock for sgdisk command, simulating partition checks
sgdisk() {
  # Simple mock for sgdisk
  if echo "$*" | grep -q "\-p"; then
    echo "${MOCK_SGDISK_PARTITIONS}"
  elif echo "$*" | grep -q "\-v"; then
    echo "${MOCK_SGDISK_VERIFY}"
  elif echo "$*" | grep -q "\-F"; then
    echo "2048"
  elif echo "$*" | grep -q "\-E"; then
    echo "1000000"
  else
    # echo "Mock sgdisk called with $*" >&2
    return 0
  fi
}

# Mock for partprobe command
partprobe() {
  return 0
}

# Mock for mkfs.ext4 command
mkfs.ext4() {
  return 0
}

# Mock for mktemp command
mktemp() {
  echo "/tmp/mock_temp_dir"
}

# Mock for mount command
mount() {
  return 0
}

# Mock for umount command
umount() {
  return 0
}

# Mock for trap_push shim
trap_push() {
  :
}

# Mock for grub-install
grub-install() {
  echo "MOCK_GRUB_INSTALL: $*"
}

# --- Tests ---

#######################################
# Verifies init_console_settings uses default values when no args present.
# Globals:
#   DEFAULT_PLATFORM_CONF
# Arguments:
#   None
# Returns:
#   0 on success, exits on failure
#######################################
test_init_console_settings_defaults() {
  echo "test_init_console_settings_defaults..."
  MOCK_CMDLINE="BOOT_IMAGE=/boot/vmlinuz"

  # Source the file in a subshell to avoid polluting test environment
  if (
    source "${DEFAULT_PLATFORM_CONF}"
    init_console_settings

    if [[ "${CONSOLE_PORT}" != "0x3f8" ]]; then
       echo "FAIL: Expected CONSOLE_PORT=0x3f8, got ${CONSOLE_PORT}"
       exit 1
    fi
    if [[ "${CONSOLE_DEV}" != "0" ]]; then
       echo "FAIL: Expected CONSOLE_DEV=0, got ${CONSOLE_DEV}"
       exit 1
    fi
    if [[ "${CONSOLE_SPEED}" != "9600" ]]; then
       echo "FAIL: Expected CONSOLE_SPEED=9600, got ${CONSOLE_SPEED}"
       exit 1
    fi
  ); then
    echo "PASS"
  else
    echo "FAILED"
    exit 1
  fi
}

#######################################
# Verifies init_console_settings parses custom values correctly.
# Globals:
#   DEFAULT_PLATFORM_CONF
# Arguments:
#   None
# Returns:
#   0 on success, exits on failure
#######################################
test_init_console_settings_custom() {
  echo "test_init_console_settings_custom..."
  MOCK_CMDLINE="console=ttyS1,115200n8"

  if (
    source "${DEFAULT_PLATFORM_CONF}"
    init_console_settings

    if [[ "${CONSOLE_PORT}" != "0x2f8" ]]; then
       echo "FAIL: Expected CONSOLE_PORT=0x2f8, got ${CONSOLE_PORT}"
       exit 1
    fi
    if [[ "${CONSOLE_DEV}" != "1" ]]; then
       echo "FAIL: Expected CONSOLE_DEV=1, got ${CONSOLE_DEV}"
       exit 1
    fi
    if [[ "${CONSOLE_SPEED}" != "115200" ]]; then
       echo "FAIL: Expected CONSOLE_SPEED=115200, got ${CONSOLE_SPEED}"
       exit 1
    fi
  ); then
    echo "PASS"
  else
    echo "FAILED"
    exit 1
  fi
}
#######################################
# Verifies find_device_from_partdev logic using mocked sysfs.
# Globals:
#   DEFAULT_PLATFORM_CONF
# Arguments:
#   None
# Returns:
#   0 on success, exits on failure
#######################################
test_find_device_from_partdev() {
  echo "test_find_device_from_partdev..."
  if (
    source "${DEFAULT_PLATFORM_CONF}"

    # Create mock sysfs structure
    local mock_sys
    mock_sys=$(mktemp -d)

    # Structure: [SYSFS_BASE]/nvme0n1p1 -> pointing to parent logic
    # Real sysfs: /sys/class/block/nvme0n1p1 -> ../../devices/virtual/nvme0n1/nvme0n1p1
    # Function Logic: cd [SYSFS_BASE]/nvme0n1p1; cd ..; echo ${PWD##*/}
    # So we need: [SYSFS_BASE]/nvme0n1p1 to be a directory inside a parent directory named 'nvme0n1'

    mkdir -p "${mock_sys}/nvme0n1/nvme0n1p1"

    # So to mock this:
    # 1. Create a directory named 'nvme0n1' (the parent).
    # 2. Create the child directory inside it 'nvme0n1/nvme0n1p1' (the real target).
    # 3. Create 'sys_base' directory.
    # 4. Create a SYMLINK in 'sys_base/nvme0n1p1' pointing to '.../nvme0n1/nvme0n1p1'.

    local true_parent="${mock_sys}/devices/nvme0n1"
    mkdir -p "${true_parent}/nvme0n1p1"

    local sys_base="${mock_sys}/class/block"
    mkdir -p "${sys_base}"
    ln -s "${true_parent}/nvme0n1p1" "${sys_base}/nvme0n1p1"

    export SYSFS_BASE="${sys_base}"

    res=$(find_device_from_partdev "/dev/nvme0n1p1")

    if [[ "$res" != "/dev/nvme0n1" ]]; then
       echo "FAIL: Expected /dev/nvme0n1 for partdev nvme0n1p1, got $res"
       exit 1
    fi

    rm -rf "${mock_sys}"
  ); then
    echo "PASS"
  else
    echo "FAILED"
    exit 1
  fi
}

#######################################
# Verifies that make_partition_dev correctly handles suffix logic.
# Globals:
#   DEFAULT_PLATFORM_CONF
# Arguments:
#   None
# Returns:
#   0 on success, exits on failure
#######################################
test_make_partition_dev() {
  echo "test_make_partition_dev..."
  if (
    source "${DEFAULT_PLATFORM_CONF}"

    res=$(make_partition_dev "/dev/nvme0n1" "1")
    if [[ "$res" != "/dev/nvme0n1p1" ]]; then echo "FAIL: nvme0n1 -> $res"; exit 1; fi

    res=$(make_partition_dev "/dev/sda" "1")
    if [[ "$res" != "/dev/sda1" ]]; then echo "FAIL: sda -> $res"; exit 1; fi

    res=$(make_partition_dev "/dev/loop0" "2")
    if [[ "$res" != "/dev/loop0p2" ]]; then echo "FAIL: loop0 -> $res"; exit 1; fi
  ); then
    echo "PASS"
  else
    echo "FAILED"
    exit 1
  fi
}



#######################################
# Verifies disk_needs_formatting logic.
# Globals:
#   DEFAULT_PLATFORM_CONF
# Arguments:
#   None
# Returns:
#   0 on success, exits on failure
#######################################
test_disk_needs_formatting() {
  echo "test_disk_needs_formatting..."
  if (
    source "${DEFAULT_PLATFORM_CONF}"
    trap_push() { :; }

    # Act & Assert Case 1: discovery_dev present
    discovery_dev="/dev/sda2"
    if disk_needs_formatting; then
       echo "FAIL: Expected return 1 (false) when discovery_dev is present"
       exit 1
    fi

    # Act & Assert Case 2: discovery_dev absent, sgdisk says valid
    unset discovery_dev
    blk_dev="/dev/sda"

    sgdisk() {
       # mock valid table
       echo "Disk entries is 500 bytes"
       return 0
    }

    if disk_needs_formatting; then
       echo "FAIL: Expected return 1 (false) when partition table is valid"
       exit 1
    fi

    # Act & Assert Case 3: discovery_dev absent, sgdisk says invalid
    sgdisk() {
       # mock invalid table (entries is 0 bytes)
       echo "Disk entries is 0 bytes"
       return 0
    }

    if ! disk_needs_formatting; then
       echo "FAIL: Expected return 0 (true) when partition table is invalid"
       exit 1
    fi
  ); then
    echo "PASS"
  else
    echo "FAILED"
    exit 1
  fi
}

#######################################
# Verifies create_partition logic in ONIE environment.
#
# Partition Layout Before:
#   - /dev/sda2: ONIE-BOOT (Mocked via blkid/mounts)
#
# Expected Outcome:
#   - Detects correct block device from partition.
#   - Calls create_sonie_uefi_partition (since EFI vars exist).
#
# Globals:
#   DEFAULT_PLATFORM_CONF
#   MOCK_BLKID
#   MOCK_MOUNTS
# Arguments:
#   None
# Returns:
#   0 on success, exits on failure
#######################################
test_create_partition_onie_dev() {
  echo "test_create_partition_onie_dev..."

  install_env="onie"
  onie_bin="echo" # mock
  MOCK_BLKID="/dev/sda2: LABEL=\"ONIE-BOOT\" UUID=\"...\""
  # Mock -o export format
  MOCK_BLKID_EXPORT=$'DEVNAME=sda2\nLABEL=ONIE-BOOT\nUUID=...\nTYPE=ext4'

  MOCK_MOUNTS="/dev/sda2 / ext4 rw 0 0"

  output=$(
    source "${DEFAULT_PLATFORM_CONF}"
    trap_push() { :; }

    # Mock create_sonie_uefi_partition AFTER sourcing to override it
    create_sonie_uefi_partition() {
      echo "CALLED_create_sonie_uefi_partition with $1"
    }

    # Mock find_device_from_partdev to avoid sysfs dependency
    find_device_from_partdev() {
        echo "/dev/sda"
    }

    # We need to simulate the environment variables usually set by install.sh or platform.conf
    install_env="onie"
    # Mock sysfs check workaround
    # If /sys/firmware/efi/efivars exists, we expect success.

    create_partition 2>&1
  )

  if [[ -d "/sys/firmware/efi/efivars" ]]; then
     if echo "$output" | grep -q "CALLED_create_sonie_uefi_partition"; then
       echo "PASS (EFI context verified)"
     else
       echo "FAIL (EFI detected but mock not called): $output"
       exit 1
     fi
  else
     if echo "$output" | grep -q "Error: BIOS is unsupported"; then
       echo "PASS (BIOS unsupported verified)"
     else
       echo "FAIL (Expected BIOS error): $output"
       exit 1
     fi
  fi
}

#######################################
# Verifies GPT partition logic, specifically XBOOTLDR creation behavior.
#
# Partition Layout Before:
#   - P1: ONIE-BOOT
#   - P2: SONIE-OS (Collision - wrong label/type)
#   - P3: SomeConfig
#
# Partition Layout After (Expected):
#   - P2: Deleted (Collision resolved)
#   - P4: Created as XBOOTLDR (Type ea00), Size 100MB
#
# Globals:
#   DEFAULT_PLATFORM_CONF
#   MOCK_SGDISK_PARTITIONS
# Arguments:
#   None
# Returns:
#   0 on success, exits on failure
#######################################
test_create_sonie_gpt_partition_logic() {
  echo "test_create_sonie_gpt_partition_logic..."

  sonie_volume_label="SONIE-OS"
  sonie_part_size=100
  sonie_type="OS"

  # Mock sgdisk behavior
  sgdisk() {
    if [[ "$1" == "-p" ]]; then
      # Simulating existing partitions: 1=ONIE, 2=SONIE, 3=Config
      echo "1 2500"
      echo "2 3500 SONIE-OS"
      echo "3 4500"
    elif [[ "$1" == "-F" ]]; then
      echo "34"
    elif [[ "$1" == "-E" ]]; then
      echo "10000"
    elif [[ "$1" == "-d" ]]; then
      echo "Deleted $1"
    else
      return 0
    fi
  }
  output=$(
    source "${DEFAULT_PLATFORM_CONF}"
    trap_push() { :; }
    create_sonie_gpt_partition "/dev/sda" 2>&1
  )
  # Should delete partition 2
  [[ "$output" == *"deleting partition 2"* ]] || { echo "FAIL: Did not delete partition 2. Output: $output"; exit 1; }
  [[ "$output" == *"Partition #4 is available"* ]] || { echo "FAIL: Did not find partition 4 available. Output: $output"; exit 1; }
  [[ "$output" == *"Creating new XBOOTLDR partition /dev/sda4 (Type: ea00)"* ]] || { echo "FAIL: Did not create XBOOTLDR partition 4. Output: $output"; exit 1; }
  echo "PASS"
}

#######################################
# Verifies re-use of valid XBOOTLDR partitions.
#
# Partition Layout Before:
#   - P1: ONIE-BOOT
#   - P2: XBOOTLDR (Reusable - correct label/type/size)
#
# Partition Layout After (Expected):
#   - P2: Reused (Attributes updated)
#   - No new partition created.
#
# Globals:
#   DEFAULT_PLATFORM_CONF
# Arguments:
#   None
# Returns:
#   0 on success, exits on failure
#######################################
test_create_sonie_gpt_partition_reuse() {
  echo "test_create_sonie_gpt_partition_reuse..."

  sonie_volume_label="SONIE-OS"
  sonie_part_size=100
  sonie_type="OS"

  # Mock sgdisk
  sgdisk() {
    local cmd="$1"
    if [[ "$cmd" == "-p" ]]; then
       echo "1 2500"
       echo "2 3500 XBOOTLDR"
    elif [[ "$cmd" == "-i" ]]; then
       if [[ "$2" == "2" ]]; then
         echo "Partition name: 'XBOOTLDR'"
         echo "Partition size: 206848 sectors (101.0 MiB)"
       fi
    elif [[ "$cmd" == "-d" ]]; then
       echo "Deleted $2"
    elif [[ "$cmd" == "--attributes=2"* ]]; then
       echo "Updated attributes for 2"
    else
       return 0
    fi
  }
  output=$(
    source "${DEFAULT_PLATFORM_CONF}"
    trap_push() { :; }
    create_sonie_gpt_partition "/dev/sda" 2>&1
  )
  # Should NOT delete partition 2
  if echo "$output" | grep -q "deleting partition 2"; then
    echo "FAIL: Should not have deleted partition 2. Output: $output"
    exit 1
  fi
  # Should reuse partition 2
  if ! echo "$output" | grep -q "Reusing existing XBOOTLDR partition 2"; then
    echo "FAIL: Did not reuse partition 2. Output: $output"
    exit 1
  fi
  # Should update attributes
  if ! echo "$output" | grep -q "Updating reused partition 2 attributes"; then
    echo "FAIL: Did not update attributes for reused partition. Output: $output"
    exit 1
  fi
  echo "PASS"
}

#######################################
# Verifies create_partition logic in SONIE environment.
#
# Environment Before:
#   - install_env="sonie"
#   - UEFI environment detected /sys/firmware/efi/efivars
#
# Expected Outcome:
#   - Calls create_sonie_uefi_partition.
#
# Globals:
#   DEFAULT_PLATFORM_CONF
# Arguments:
#   None
# Returns:
#   0 on success, exits on failure
#######################################
test_create_partition_sonie() {
  echo "test_create_partition_sonie..."

  install_env="sonie"
  onie_bin=""
  firmware="uefi" # Assume UEFI for SONiC

  output=$(
    source "${DEFAULT_PLATFORM_CONF}"
    trap_push() { :; }

    # Mock find_device_from_partdev
    find_device_from_partdev() { echo "/dev/sda"; }

    # Mock blkid -o export
    blkid() {
      if [[ "$1" == "-o" ]] && [[ "$2" == "export" ]]; then
         # Simulate finding ONIE-BOOT which leads to /dev/sda
         echo $'DEVNAME=sda2\nLABEL=ONIE-BOOT'
      fi
    }

    create_sonie_uefi_partition() {
      echo "CALLED_create_sonie_uefi_partition with $1"
    }
    # Mock mkfs.vfat
    mkfs.vfat() { echo "mkfs.vfat $*"; }
    # Mock mount
    mount() { echo "mount $*"; }

    # Mock sysfs/efivars presence for UEFI path
    mkdir -p /tmp/mock_temp_dir/sys/firmware/efi/efivars
    local efivars_dir="/tmp/mock_temp_dir/sys/firmware/efi/efivars"

    create_partition 2>&1
  )

  if [[ -d "/sys/firmware/efi/efivars" ]]; then
     if echo "$output" | grep -q "CALLED_create_sonie_uefi_partition"; then
       echo "PASS"
     else
       echo "FAIL: $output"
       exit 1
     fi
  else
     # If BIOS, it errors out. Verify error message.
     if echo "$output" | grep -q "Error: BIOS is unsupported"; then
       echo "PASS (BIOS unsupported)"
     else
       echo "FAIL (Expected BIOS error): $output"
       exit 1
     fi
  fi
  rm -rf /tmp/mock_temp_dir
}

#######################################
# Verifies create_partition logic in SONiC environment.
#
# Environment Before:
#   - install_env="sonic"
#   - ONIE-BOOT partition exists (/dev/sda3)
#
# Expected Outcome:
#   - Detects block device /dev/sda.
#   - Calls create_sonie_uefi_partition.
#
# Globals:
#   DEFAULT_PLATFORM_CONF
# Arguments:
#   None
# Returns:
#   0 on success, exits on failure
#######################################
test_create_partition_sonic() {
  echo "test_create_partition_sonic..."

  output=$(
    source "${DEFAULT_PLATFORM_CONF}"
    trap_push() { :; }

    # Mock environment
    install_env="sonic"
    onie_bin=""
    firmware="uefi" # Assume UEFI for SONiC

    # Mock commands
    blkid() {
      if [[ "$1" == "-o" ]] && [[ "$2" == "export" ]]; then
        echo $'DEVNAME=sda3\nLABEL=ONIE-BOOT\nTYPE=ext4'
      else
        echo "/dev/sda3: UUID=\"...\" TYPE=\"ext4\" PARTLABEL=\"ONIE-BOOT\""
      fi
    }
    find_device_from_partdev() { echo "/dev/sda"; }
    # Mock create_sonie_uefi_partition
    create_sonie_uefi_partition() { echo "create_sonie_uefi_partition called with $1"; }
    # Mock onie-sysinfo
    onie-sysinfo() { echo "gpt"; }

    create_partition 2>&1
  )

  [[ "$output" == *"create_sonie_uefi_partition called with /dev/sda"* ]] || { echo "FAIL: $output"; exit 1; }
  echo "PASS"
}

# Run tests
#######################################
# Verifies that create_sonie_uefi_partition cleans up legacy boot variables.
#
# Boot Entries Before:
#   - Boot0001: SONIE-OS
#   - Boot0002: ACS-OS
#   - Boot0003: SONiC-OS
#   - Boot0004: Other-OS
#   - Boot0005: ONIE
#
# Boot Entries After (Expected):
#   - Boot0001, Boot0002, Boot0003, Boot0005: Deleted
#   - Boot0004: Remains independent
#
# Globals:
#   DEFAULT_PLATFORM_CONF
# Arguments:
#   None
# Returns:
#   0 on success, exits on failure
#######################################
test_create_sonie_uefi_partition_cleanup() {
  echo "test_create_sonie_uefi_partition_cleanup..."

  sonie_volume_label="SONIE-OS"

  # Run in subshell to isolate mocks
  output=$(
    source "${DEFAULT_PLATFORM_CONF}"
    trap_push() { :; }

    # Mock create_sonie_gpt_partition to do nothing
    create_sonie_gpt_partition() { :; }

    # Mock efibootmgr
    efibootmgr() {
      if [[ "$1" == "-b" ]]; then
         echo "DELETING_BOOT_ENTRY $2" >> /tmp/mock_efibootmgr_log
         return 0
      fi
      # Simulate existing entries
      echo "Boot0001* SONIE-OS"
      echo "Boot0002* ACS-OS"
      echo "Boot0003* SONiC-OS"
      echo "Boot0004* Other-OS"
      echo "Boot0005* ONIE"
    }

    rm -f /tmp/mock_efibootmgr_log
    create_sonie_uefi_partition "/dev/sda" 2>&1
  )

  local log_content
  log_content=$(cat /tmp/mock_efibootmgr_log 2>/dev/null)

  if ! echo "$log_content" | grep -q "DELETING_BOOT_ENTRY 0001"; then echo "FAIL: Missed SONIE-OS"; exit 1; fi
  if ! echo "$log_content" | grep -q "DELETING_BOOT_ENTRY 0002"; then echo "FAIL: Missed ACS-OS"; exit 1; fi
  if ! echo "$log_content" | grep -q "DELETING_BOOT_ENTRY 0003"; then echo "FAIL: Missed SONiC-OS"; exit 1; fi
  if ! echo "$log_content" | grep -q "DELETING_BOOT_ENTRY 0005"; then echo "FAIL: Missed ONIE"; exit 1; fi
  if echo "$log_content" | grep -q "DELETING_BOOT_ENTRY 0004"; then echo "FAIL: Deleted Other-OS"; exit 1; fi

  rm -f /tmp/mock_efibootmgr_log
  echo "PASS"
}

#######################################
# Verifies install_grub_to_esp functionality.
#
# Filesystem Before:
#   - ESP: Empty (mocked)
#   - OS: /os/linux.efi present
#
# Filesystem After (Expected):
#   - GRUB installed to ESP (via mock call check).
#   - /os/grub/grub.cfg created with chainloader entry for /linux.efi.
#
# Globals:
#   DEFAULT_PLATFORM_CONF
# Arguments:
#   None
# Returns:
#   0 on success, exits on failure
#######################################
test_install_grub_to_esp() {
  echo "test_install_grub_to_esp..."

  local mock_fs_root
  mock_fs_root=$(mktemp -d)
  local mock_esp="${mock_fs_root}/efi"
  local mock_os="${mock_fs_root}/os"

  mkdir -p "$mock_esp"
  mkdir -p "$mock_os"
  mkdir -p "$mock_os/grub"

  # create dummy UKI
  touch "$mock_os/linux.efi"

  # Create a dummy bootloader state machine file
  local mock_state_machine="${mock_fs_root}/bootloader_state_machine.grub"
  echo "# Mock State Machine" > "$mock_state_machine"

  output=$(
    source "${DEFAULT_PLATFORM_CONF}"
    trap_push() { :; }

    # Mock sgdisk -p for EF00 detection
    sgdisk() {
      if [[ "$1" == "-p" ]]; then
        echo "1 2048 4096 200MB EF00 EFI System"
      fi
    }

    # Mock /proc/cpuinfo
    # We can mock this either by creating a file if we can control where the script looks,
    # but the script looks at /proc/cpuinfo directly.
    # We can skip mocking it if we accept default CSTATES behavior (empty).

    sonie_volume_label="SONIE-TEST"
    bootloader_state_machine="$mock_state_machine"

    # Mock defaults for serial
    CONSOLE_PORT="0x3f8"
    CONSOLE_SPEED="115200"

    install_grub_to_esp "/dev/sda" "$mock_esp" "$mock_os"
  )


  # Verify grub-install called
  if ! echo "$output" | grep -q "MOCK_GRUB_INSTALL: --target=x86_64-efi --efi-directory=$mock_esp --boot-directory=$mock_os --bootloader-id=SONIE-TEST --recheck --modules=part_gpt part_msdos /dev/sda"; then
    echo "FAIL: grub-install not called with expected args. Output: $output"
    rm -rf "$mock_fs_root"
    exit 1
  fi
  # Verify grub.cfg content
  local generated_cfg="${mock_os}/grub/grub.cfg"
  if [ ! -f "$generated_cfg" ]; then
    echo "FAIL: grub.cfg not created at $generated_cfg"
    rm -rf "$mock_fs_root"
    exit 1
  fi

  local cfg_content
  cfg_content=$(cat "$generated_cfg")

  # Verify Chainloader
  if ! echo "$cfg_content" | grep -q "chainloader /linux.efi"; then
    echo "FAIL: grub.cfg missing chainloader. Content: $cfg_content"
    rm -rf "$mock_fs_root"
    exit 1
  fi

  # Verify Serial Config
  if ! echo "$cfg_content" | grep -q "serial --port=0x3f8 --speed=115200"; then
    echo "FAIL: grub.cfg missing serial config. Content: $cfg_content"
    rm -rf "$mock_fs_root"
    exit 1
  fi

  # Verify State Machine Logic
  if ! echo "$cfg_content" | grep -q "Mock State Machine"; then
    echo "FAIL: grub.cfg missing state machine content. Content: $cfg_content"
    rm -rf "$mock_fs_root"
    exit 1
  fi
  if ! echo "$cfg_content" | grep -q "load_state"; then
     echo "FAIL: grub.cfg missing state machine calls. Content: $cfg_content"
     rm -rf "$mock_fs_root"
     exit 1
  fi

  rm -rf "$mock_fs_root"
  echo "PASS"
}

#######################################
# Main test harness execution.
#######################################
main() {
  test_init_console_settings_defaults
  test_init_console_settings_custom
  test_find_device_from_partdev
  test_make_partition_dev
  test_disk_needs_formatting
  test_create_partition_onie_dev
  test_create_sonie_gpt_partition_logic
  test_create_sonie_gpt_partition_reuse
  test_create_partition_sonie
  test_create_partition_sonic
  test_create_sonie_uefi_partition_cleanup
  test_install_grub_to_esp


  rm -f /tmp/mock_temp_dir
}

main "$@"
