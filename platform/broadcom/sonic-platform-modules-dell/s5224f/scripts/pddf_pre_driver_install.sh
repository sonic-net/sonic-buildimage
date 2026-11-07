#!/bin/bash
# Pre-driver-install hook for PDDF mode on Dell S5224F.
#
# Removes BSP-specific modules that conflict with PDDF drivers.
# pddf_util.py calls this before loading the 21 standard kernel modules
# and PDDF modules.  The standard modules (i2c_ismt, i2c-i801, ipmi_si,
# i2c_dev, etc.) are reloaded by pddf_util.py immediately afterwards;
# only the BSP FPGA modules stay removed.

# --- BSP FPGA modules (conflict with pddf_fpgapci_driver) ----------------
modprobe -r dell_s5224f_fpga_ocores 2>/dev/null
modprobe -r i2c_ocores              2>/dev/null

# --- I2C providers (pddf_util.py reloads them as standard modules) --------
modprobe -r i2c_ismt       2>/dev/null
modprobe -r i2c-i801       2>/dev/null

# --- IPMI: quiesce kipmid before removing ipmi_si ------------------------
# ipmi_si starts a kernel thread (kipmid) that busy-polls at
# kipmid_max_busy_us microseconds.  If we modprobe -r ipmi_si while
# kipmid is active, the removal fails and leaks SMBus/ACPI references
# that skew the I2C adapter topology across a mode switch.
modprobe -r acpi_ipmi      2>/dev/null
modprobe -r ipmi_ssif      2>/dev/null
echo 0 > /sys/module/ipmi_si/parameters/kipmid_max_busy_us 2>/dev/null
modprobe -r ipmi_si        2>/dev/null
modprobe -r ipmi_devintf   2>/dev/null

# --- Other I2C modules ----------------------------------------------------
modprobe -r i2c-mux-pca954x 2>/dev/null
modprobe -r i2c-dev         2>/dev/null

# --- Verify BSP FPGA module is removed (critical for PDDF) ----------------
if lsmod | grep -q '^dell_s5224f_fpga_ocores '; then
    sleep 0.5
    modprobe -r dell_s5224f_fpga_ocores 2>/dev/null
    if lsmod | grep -q '^dell_s5224f_fpga_ocores '; then
        echo "WARNING: dell_s5224f_fpga_ocores still loaded - PDDF may have I/O conflicts" >&2
    fi
fi
