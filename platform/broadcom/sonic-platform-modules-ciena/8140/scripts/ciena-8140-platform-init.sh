#!/bin/bash
# Platform initialization script for Ciena CN8140 (Rudra40)
# This script initializes FPGA optics control registers and
# binds the optoe driver to all transceiver I2C buses.
#
# I2C Bus Topology (confirmed from hardware):
#   PCI 0000:19:00.0 - Rudra40 CFPGA (16fc:033b)
#     rudra40_main_i2c      → i2c-30  (BCM DRAM tuning)
#     rudra40_more_i2c_pwrgd → i2c-54
#     rudra40_more_i2c_j2c_ioexp → i2c-70
#     sfp0                   → i2c-40  (optics parent bus)
#       fpga_mux channels    → i2c-101..140  (40x SFP28)
#       fpga_mux channels    → i2c-301..308  (8x QSFP28)
#       i2c-49               → i2c-401..408  (QSFP mgmt)
#
#   PCH SMBus (0000:00:1f.4) → i2c-0
#     sutra (i2c-PRP0001:00)
#       sutra_main_i2c       → i2c-31  (board EEPROM, ADC, USB mux)
#       sutra_ps_i2c         → i2c-32  (PSU mux)
#         i2c-50: PSU-A (eeprom, pmbus)
#         i2c-51: PSU-B (eeprom, pmbus)
#       sutra_pcie_i2c       → i2c-52
#       sutra_pmbus_i2c      → i2c-55  (VCCIN pmbus)

PLATFORM="CN8140"
LOG_TAG="ciena-8140-init"
PLREG="/usr/local/bin/plreg"
CHARDEV_SYMLINK_DIR="/usr/share/sonic/device/x86_64-ciena-8140-r0/chardev"

# Logging function
log_info() {
    logger -t "$LOG_TAG" -p user.info "$1"
    echo "$1"
}

log_error() {
    logger -t "$LOG_TAG" -p user.err "$1"
    echo "ERROR: $1" >&2
}

# Create stable symlinks for raw_chardev devices.
# The kernel's platform_device auto-numbering (ciena_raw_chardev.N.auto)
# is non-deterministic — the base N can shift between boots or across
# different units.  The FPGA driver always creates the MFG IDP EEPROM
# first, followed by the 6 fan tray EEPROMs in order, so we sort
# numerically and assign stable names.
#
# Symlinks created under /run/ciena/:
#   mfg_eeprom     → first raw_chardev  (system MFG IDP EEPROM)
#   fan_eeprom_1   → second raw_chardev (fan tray 1, fans 1-2)
#   fan_eeprom_2   → third  raw_chardev (fan tray 2, fans 3-4)
#   ...
#   fan_eeprom_6   → seventh raw_chardev (fan tray 6, fans 11-12)
create_chardev_symlinks() {
    local pci_path="/sys/devices/pci0000:00/0000:00:10.0/0000:03:00.0/0000:04:01.0"
    local devs

    # Collect all ciena_raw_chardev.<N>.auto directories, sorted numerically
    # by the auto-numbering index <N>. 
    devs=$(for d in "$pci_path"/ciena_raw_chardev.*.auto; do
        [ -e "$d" ] || continue
        n=$(basename "$d" | sed -n 's/^ciena_raw_chardev\.\([0-9]\+\)\.auto$/\1/p')
        [ -n "$n" ] && printf '%s\t%s\n' "$n" "$d"
    done | sort -n -k1,1 | cut -f2-)

    if [ -z "$devs" ]; then
        log_error "No ciena_raw_chardev devices found under $pci_path"
        return 1
    fi

    mkdir -p "$CHARDEV_SYMLINK_DIR"

    local index=0
    for dev in $devs; do
        local chardev_file="$dev/raw_chardev"
        if [ ! -f "$chardev_file" ]; then
            log_error "Missing raw_chardev file in $dev"
            continue
        fi

        if [ $index -eq 0 ]; then
            ln -sf "$chardev_file" "$CHARDEV_SYMLINK_DIR/mfg_eeprom"
            log_info "Symlink: mfg_eeprom -> $(basename "$dev")/raw_chardev"
        else
            ln -sf "$chardev_file" "$CHARDEV_SYMLINK_DIR/fan_eeprom_${index}"
            log_info "Symlink: fan_eeprom_${index} -> $(basename "$dev")/raw_chardev"
        fi
        index=$((index + 1))
    done

    log_info "Created $index raw_chardev symlinks in $CHARDEV_SYMLINK_DIR"
    return 0
}

# Wait for I2C buses to be available
wait_for_i2c_buses() {
    local timeout=30
    local count=0

    log_info "Waiting for I2C buses to be created..."

    while [ $count -lt $timeout ]; do
        if [ -e /dev/i2c-101 ]; then
            log_info "I2C buses are ready"
            return 0
        fi
        sleep 1
        count=$((count + 1))
    done

    log_error "Timeout waiting for I2C buses"
    return 1
}

# Wait for FPGA PCI driver to probe successfully.
# We must NOT touch the FPGA via plreg (/dev/mem mmap to BAR0) until a
# kernel driver has bound — raw userspace MMIO before the FPGA's internal
# logic is ready triggers a PCI SERR NMI → kernel panic.
wait_for_fpga_ready() {
    local timeout=30
    local count=0
    local pci_dev=""

    log_info "Waiting for Rudra40 FPGA driver to probe..."

    while [ $count -lt $timeout ]; do
        # Find the Rudra40 PCI device (vendor 16fc, device 033b)
        for d in /sys/bus/pci/devices/*; do
            [ -f "$d/vendor" ] || continue
            if [ "$(cat "$d/vendor" 2>/dev/null)" = "0x16fc" ] && \
               [ "$(cat "$d/device" 2>/dev/null)" = "0x033b" ]; then
                pci_dev="$d"
                break
            fi
        done

        if [ -n "$pci_dev" ] && [ -d "$pci_dev/driver" ]; then
            local drv
            drv=$(basename "$(readlink "$pci_dev/driver")" 2>/dev/null)
            log_info "FPGA driver bound: $drv at $pci_dev (after ${count}s)"
            return 0
        fi
        sleep 1
        count=$((count + 1))
    done

    log_error "Timeout: Rudra40 FPGA driver not bound after ${timeout}s"
    return 1
}

# Initialize FPGA optics control registers
init_fpga_optics() {
    log_info "Initializing FPGA optics control registers..."

    if [ ! -x "$PLREG" ]; then
        log_error "plreg utility not found at $PLREG"
        return 1
    fi

    # Ensure the FPGA is ready to accept register writes.
    if ! wait_for_fpga_ready; then
        log_error "FPGA not accessible; skipping optics init to avoid PCI SERR"
        return 1
    fi

    # Deassert QSFP reset one port at a time to avoid simultaneous
    # transceiver inrush / PCI bus contention.
    # Rudra40 has 8 QSFP ports (bits[7:0])
    log_info "Deasserting QSFP reset (per-port staggered)..."
    local mask=0
    for bit in 1 2 4 8 16 32 64 128; do
        mask=$((mask | bit))
        $PLREG write RUDRA40_OPTICS_QSFP_RESET_0 $(printf '0x%02X' $mask)
        sleep 0.2
    done

    # Small delay for QSFPs to come out of reset and initialize
    sleep 1

    # Take QSFPs out of low-power mode so EEPROM is fully accessible
    log_info "Clearing QSFP low-power mode (all ports)..."
    $PLREG write RUDRA40_OPTICS_QSFP_LOW_PWR_0 0x00

    # Enable SFP TX for all ports (active-low: 0 = TX enabled)
    log_info "Enabling SFP TX (all ports)..."
    $PLREG write RUDRA40_OPTICS_SFP_TX_DISABLE 0x00000000
    $PLREG write RUDRA40_OPTICS_SFP_TX_DISABLE_2 0x00000000

    # Log current status
    log_info "QSFP present: $($PLREG read RUDRA40_OPTICS_STATUS_QSFP_PRESENT_0 2>/dev/null | grep 'Value:' | awk '{print $2}')"
    log_info "SFP present:  $($PLREG read RUDRA40_OPTICS_STATUS_SFP_PRESENT 2>/dev/null | grep 'Value:' | awk '{print $2}')"

    log_info "FPGA optics initialization complete"
    return 0
}

# Load optoe driver
load_optoe_driver() {
    log_info "Loading optoe driver..."

    if lsmod | grep -q "^optoe "; then
        log_info "optoe driver already loaded"
        return 0
    fi

    modprobe optoe
    if [ $? -eq 0 ]; then
        log_info "optoe driver loaded successfully"
        return 0
    else
        log_error "Failed to load optoe driver"
        return 1
    fi
}

# Bind optoe to SFP ports
bind_sfp_ports() {
    local start_bus=101
    local end_bus=140
    local bus

    log_info "Binding optoe2 to SFP ports (buses $start_bus-$end_bus)..."

    for bus in $(seq $start_bus $end_bus); do
        if [ -e /sys/bus/i2c/devices/i2c-$bus ]; then
            # Check if already bound
            if [ -e /sys/bus/i2c/devices/$bus-0050 ]; then
                log_info "Bus $bus already has device at 0x50, skipping"
                continue
            fi

            # Bind optoe2 (for SFP/SFP+/SFP28)
            echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-$bus/new_device 2>/dev/null
            if [ $? -eq 0 ]; then
                log_info "Bound optoe2 to bus $bus"
            else
                log_error "Failed to bind optoe2 to bus $bus"
            fi
        else
            log_error "I2C bus $bus does not exist"
        fi
    done
}

# Bind optoe to QSFP-DD ports
bind_qsfp_ports() {
    local start_bus=301
    local end_bus=308

    log_info "Binding optoe3 to QSFP-DD ports (buses $start_bus-$end_bus)..."

    for bus in $(seq $start_bus $end_bus); do
        if [ -e /sys/bus/i2c/devices/i2c-$bus ]; then
            # Check if already bound
            if [ -e /sys/bus/i2c/devices/$bus-0050 ]; then
                log_info "Bus $bus already has device at 0x50, skipping"
                continue
            fi

            # Bind optoe3 (for QSFP-DD / CMIS)
            echo optoe3 0x50 > /sys/bus/i2c/devices/i2c-$bus/new_device 2>/dev/null
            if [ $? -eq 0 ]; then
                log_info "Bound optoe3 to bus $bus"
            else
                log_error "Failed to bind optoe3 to bus $bus"
            fi
        else
            log_error "I2C bus $bus does not exist"
        fi
    done
}

# Verify EEPROM files are accessible
verify_eeprom_access() {
    local test_bus=101
    local eeprom_path="/sys/bus/i2c/devices/$test_bus-0050/eeprom"

    log_info "Verifying EEPROM access..."

    if [ -e "$eeprom_path" ]; then
        if [ -r "$eeprom_path" ]; then
            log_info "EEPROM is accessible at $eeprom_path"
            return 0
        else
            log_error "EEPROM exists but is not readable at $eeprom_path"
            return 1
        fi
    else
        log_error "EEPROM not found at $eeprom_path"
        return 1
    fi
}

# Enable BMC8140 FPGA PCI device
# The BMC8140 (16fc:0340) manages board thermal sensors (9x TMP421),
# fans, and PSU monitoring.  It has no kernel driver in SONiC, but its
# BAR0 registers must be accessible for platform thermal management.
# Without explicitly enabling the PCI device, BAR0 reads return 0xFFFFFFFF.
enable_bmc8140() {
    local pci_dev=""

    for d in /sys/bus/pci/devices/*; do
        [ -f "$d/vendor" ] || continue
        if [ "$(cat "$d/vendor" 2>/dev/null)" = "0x16fc" ] && \
           [ "$(cat "$d/device" 2>/dev/null)" = "0x0340" ]; then
            pci_dev="$d"
            break
        fi
    done

    if [ -z "$pci_dev" ]; then
        log_error "BMC8140 PCI device [16fc:0340] not found"
        return 1
    fi

    if [ "$(cat "$pci_dev/enable" 2>/dev/null)" = "1" ]; then
        log_info "BMC8140 PCI device already enabled ($pci_dev)"
        return 0
    fi

    echo 1 > "$pci_dev/enable" 2>/dev/null
    if [ "$(cat "$pci_dev/enable" 2>/dev/null)" = "1" ]; then
        log_info "BMC8140 PCI device enabled ($pci_dev)"
        return 0
    else
        log_error "Failed to enable BMC8140 PCI device ($pci_dev)"
        return 1
    fi
}

# Ensure config_db.json is valid
# If the file is missing or empty/corrupt, generate a factory default so that
# config-setup.service can load it successfully on first boot.
ensure_config_db() {
    local cfg="/etc/sonic/config_db.json"
    local hwsku="Ciena-8140"
    local platform="x86_64-ciena-8140-r0"

    if [ -s "$cfg" ] && python3 -c "import json,sys; json.load(open('$cfg'))" 2>/dev/null; then
        log_info "config_db.json is valid"
        return 0
    fi

    log_info "config_db.json is missing or invalid — generating factory default"
    if command -v sonic-cfggen >/dev/null 2>&1; then
        sonic-cfggen -H -k "$hwsku" --preset l3 > "${cfg}.tmp" 2>/dev/null
        if [ $? -eq 0 ] && [ -s "${cfg}.tmp" ]; then
            mv "${cfg}.tmp" "$cfg"
            log_info "Generated factory config_db.json (hwsku=$hwsku)"
        else
            rm -f "${cfg}.tmp"
            log_info "sonic-cfggen generation attempt failed; config will be created by systemd services"
            return 0
        fi
    else
        log_info "sonic-cfggen not available; config will be created by systemd services"
        return 0
    fi
}

# Report/clear the FPGA reconfig "breadcrumb" left by the reboot hook.
# This is informational only -- the activation (FPGA reconfig) itself is
# performed by platform_reboot during a cold reboot. Here we just tidy up the
# marker and log the outcome so the operator can see it in syslog.
# Return the decoded value of a named field from a plreg register dump,Prints the
# field's Value column, e.g. plreg_field RUDRA40_BASE_BRD_ID fpga_load_type.
plreg_field() {
    "$PLREG" read "$1" 2>/dev/null | awk -v f="$1_$2" '$1==f {print $NF; exit}'
}

check_fpga_reconfig_state() {
    local pending="/host/fpga_reconfig_pending"
    local attempted="/host/fpga_reconfig_attempted"
    local mjr mnr bld ver load_type
    if [ -f "$attempted" ]; then
        mjr=$("$PLREG" -c read RUDRA40_BASE_MJR 2>/dev/null)
        mnr=$("$PLREG" -c read RUDRA40_BASE_MNR 2>/dev/null)
        bld=$("$PLREG" -c read RUDRA40_BASE_BLD 2>/dev/null)
        ver="$((${mjr:-0})).$((${mnr:-0})).$((${bld:-0}))"
        # Confirm activation by which bank the control FPGA is running.
        # fpga_load_type: 1 = the flashed USER bank is live (activation OK),
        # 0 = fell back to the read-only GOLDEN bank (user image rejected).
        load_type=$(plreg_field RUDRA40_BASE_BRD_ID fpga_load_type)
        if [ "$((${load_type:-0}))" = "1" ]; then
            log_info "FPGA reconfig activation SUCCEEDED: running USER bank, version ${ver}"
        else
            log_error "FPGA reconfig activation FAILED: running GOLDEN bank (version ${ver}); the flashed user image was rejected"
        fi
        rm -f "$attempted"
    fi
    if [ -f "$pending" ]; then
        log_info "FPGA reconfig is PENDING (image flashed, not yet activated); it will activate on the next COLD reboot"
    fi
}

# Main initialization
main() {
    log_info "Starting $PLATFORM platform initialization..."

    # Tidy up / report FPGA firmware-activation breadcrumbs (see
    # platform_reboot). Non-fatal and informational only.
    check_fpga_reconfig_state

    # Ensure config_db.json is valid before config-setup.service runs.
    ensure_config_db

    # Enable BMC8140 FPGA early so thermal/fan registers are accessible
    enable_bmc8140

    # Create stable symlinks for raw_chardev devices (MFG EEPROM, fan EEPROMs)
    create_chardev_symlinks

    # Set STATUS LED to flashing green to indicate boot-in-progress.
    # NOTE: front::all maps to SUTRA_GLUE_LED_SYS_STATUS_0.enable_all_leds, which is
    # an ALL-ON lamp-test override: brightness=1 forces *every* front-panel LED on and
    # washes out the individual colour indications. It must be 0 for the per-colour
    # status/alarm/sync/gnss LEDs to be visible, so explicitly clear it here.
    if [ -d /sys/class/leds/front::all ]; then
        echo 0 > /sys/class/leds/front::all/brightness 2>/dev/null
        echo 0 > /sys/class/leds/front:yellow:alarm/blink 2>/dev/null
        echo 0 > /sys/class/leds/front:yellow:alarm/brightness 2>/dev/null
        echo 1 > /sys/class/leds/front:green:status/brightness 2>/dev/null
        echo 1 > /sys/class/leds/front:green:status/blink 2>/dev/null
        log_info "STATUS LED set to flashing green (boot in progress)"
    fi

    # Initialize FPGA optics control (reset, power, TX enable)
    if ! init_fpga_optics; then
        log_error "$PLATFORM platform initialization failed (FPGA optics init)"
        exit 1
    fi

    # Program the Rudra40 FPGA front-panel SFP/QSFP LED colour/rate registers
    # and enable the global LED output gate. The Broadcom M0 LED microcode
    # (loaded by led_proc_init.soc in the syncd container) supplies the
    # link/activity serial frame; the FPGA supplies the per-port colour.
    # Non-fatal: a LED init failure must not abort transceiver bring-up.
    LED_FPGA_INIT="/usr/share/sonic/device/x86_64-ciena-8140-r0/ciena-8140-led-fpga-init.sh"
    if [ -x "$LED_FPGA_INIT" ]; then
        "$LED_FPGA_INIT" || log_error "FPGA LED init reported errors (non-fatal)"
    else
        log_error "FPGA LED init script not found at $LED_FPGA_INIT (skipping)"
    fi

    # Load optoe driver (needed before binding)
    if ! load_optoe_driver; then
        log_error "$PLATFORM platform initialization failed (optoe driver)"
        exit 1
    fi

    # Wait for I2C buses to appear (FPGA-mediated buses 101+)
    if wait_for_i2c_buses; then
        # Bind optoe to all transceiver ports
        bind_sfp_ports
        bind_qsfp_ports

        # Verify access (informational only)
        verify_eeprom_access
    else
        log_error "I2C buses did not appear in time; skipping optoe binding"
        log_error "Transceiver detection will be retried by xcvrd"
        exit 1
    fi

    log_info "$PLATFORM platform initialization complete"
    exit 0
}

# Run main function
main
