#!/bin/bash

TRUE=0
FALSE=1

HW_EXT_ID=-1
NTM_PRESENCE=0
IO_PORT_FILE="/dev/port"

# Hardware ext id definitions
EXT_ID_0=0
EXT_ID_1=1

# Function to check file existence
function _check_filepath {
    local filepath=$1
    if [ -z "$filepath" ]; then
        echo "[ERR] The input string is empty!"
        return $FALSE
    elif [ ! -f "$filepath" ] && [ ! -c "$filepath" ]; then
        echo "[ERR] No such file: $filepath"
        return $FALSE
    else
        return $TRUE
    fi
}

# Function to detect hardware ext ID
function check_hw_ext_id {
    if _check_filepath "$IO_PORT_FILE"; then
        if ! REG=$(xxd -s 0x706 -p -l 1 -c 1 "$IO_PORT_FILE" 2>/dev/null); then
            echo "[ERR] Failed to read from $IO_PORT_FILE"
            HW_EXT_ID=-1
        else
            MASK=0x07
            HW_EXT_ID=$(( 0x$REG & MASK ))
        fi
    else
        HW_EXT_ID=-1
    fi
    echo "HW_EXT_ID detected: $HW_EXT_ID"
}

# Function to check NTM presence
function check_ntm_presence {
    check_hw_ext_id
    if [ "$HW_EXT_ID" -eq "$EXT_ID_0" ] || [ "$HW_EXT_ID" -eq "$EXT_ID_1" ]; then
        NTM_PRESENCE=1
    else
        NTM_PRESENCE=0
    fi
}

# Function _outb(u8 data, u16 port)
# Uses 'dd' to write a single byte to the physical I/O port via /dev/port
_outb() {
    local data=$1  # e.g., 0xa5
    local port=$2  # e.g., 0x2e

    # 1. Convert the port hex string (0x2e) to decimal (46) for dd's 'seek'
    local port_dec=$(($port))

    # 2. Convert data to a binary byte and pipe it to dd
    printf "$(printf '\\x%x' "$data")" | dd of=/dev/port bs=1 count=1 seek="$port_dec" conv=notrunc 2> /dev/null

    # mdelay (5ms)
    sleep 0.005
}

# Function init BMC Mailbox via /dev/port
function init_bmc_mailbox {
    local bmc_mailbox_done="/tmp/bmc_mailbox_done"

    if [ -f "$bmc_mailbox_done" ]; then
        echo "BMC Mailbox already initialized."
        return
    fi

    echo "Initializing BMC Mailbox (via /dev/port)..."

    # --- Enable super io writing ---
    _outb 0xa5 0x2e
    _outb 0xa5 0x2e

    # --- Logic device number ---
    # Select Register 0x07 (LDN)
    _outb 0x07 0x2e
    # Set LDN to 0x0e
    _outb 0x0e 0x2f

    # --- Disable mailbox ---
    # Select Register 0x30 (Activate)
    _outb 0x30 0x2e
    # Set 0x00 (Disable)
    _outb 0x00 0x2f

    # --- Set base address (0x07C0) ---
    # Select Reg 0x60 (Base Addr MSB)
    _outb 0x60 0x2e
    # Set 0x07
    _outb 0x07 0x2f

    # Select Reg 0x61 (Base Addr LSB)
    _outb 0x61 0x2e
    # Set 0xc0
    _outb 0xc0 0x2f

    # --- Select bit[3:0] of SIRQ ---
    _outb 0x70 0x2e
    _outb 0x07 0x2f

    # --- Low level trigger ---
    _outb 0x71 0x2e
    _outb 0x01 0x2f

    # --- Enable mailbox ---
    _outb 0x30 0x2e
    # Set 0x01 (Enable)
    _outb 0x01 0x2f

    # --- Disable super io writing ---
    _outb 0xaa 0x2e

    # --- Mailbox initial ---
    _outb 0x00 0x786
    _outb 0x00 0x787

    # Create a file to indicate mailbox initialization is done
    touch "$bmc_mailbox_done"

    echo "BMC Mailbox initialized."
}

function execute_post_device_init {
    local pddf_py_script="/usr/local/bin/pddf_post_device_create.py"

    if [ -f "$pddf_py_script" ]; then
        echo "Executing post device create Python script..."
        if ! python3 "$pddf_py_script"; then
            echo "[WARNING] Python script execution failed"
        fi
    fi
}

function disable_bmc_watchdog {
    echo "Disable BMC watchdog"
    timeout 3 ipmitool mc watchdog off > /dev/null 2>&1
}

function set_bmc_sel_time {
    echo "Set BMC SEL time to system time"
    timeout 3 ipmitool sel time set now > /dev/null 2>&1
}

function set_led_default_val {
    check_ntm_presence
    if [ "$NTM_PRESENCE" -eq 1 ]; then
        pddf_ledutil setstatusled DIAG_LED off
    fi

    pddf_ledutil setstatusled LOC_LED off

    #set status led to green to indicate platform init done
    curr_led=$(pddf_ledutil getstatusled SYS_LED)
    pddf_ledutil setstatusled SYS_LED green
    echo "Set SYS_LED from $curr_led to green"
}

function platform_firmware_versions() {
    FIRMWARE_VERSION_FILE=/var/log/firmware_versions
    rm -rf ${FIRMWARE_VERSION_FILE}

    cpld1_ver=$(printf '%d.%02d.%03d' \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld1_major_ver) \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld1_minor_ver) \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld1_build))
    echo "CPLD1: ${cpld1_ver}"  >> $FIRMWARE_VERSION_FILE

    cpld2_ver=$(printf '%d.%02d.%03d' \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld2_major_ver) \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld2_minor_ver) \
                $(cat /sys/kernel/pddf/devices/sysstatus/sysstatus_data/cpld2_build))
    echo "CPLD2: ${cpld2_ver}"  >> $FIRMWARE_VERSION_FILE

    bios_ver=$(cat /sys/class/dmi/id/bios_version)
    echo "BIOS: ${bios_ver}"  >> $FIRMWARE_VERSION_FILE

    VERARR=(`ipmitool raw 0x6 0x1 2> /dev/null | cut -d ' ' -f 4,5,16,15,14`)
    echo "BMC: ${VERARR[0]}.${VERARR[1]}.${VERARR[4]}.${VERARR[3]}${VERARR[2]}"  >> $FIRMWARE_VERSION_FILE
}

disable_bmc_watchdog
set_bmc_sel_time
set_led_default_val
init_bmc_mailbox
platform_firmware_versions
execute_post_device_init

echo "PDDF device post-create completed"
