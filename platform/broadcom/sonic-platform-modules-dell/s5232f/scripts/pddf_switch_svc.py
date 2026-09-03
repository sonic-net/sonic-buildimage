#!/usr/bin/env python3
"""Dell S5232F hooks for switching between PDDF and Non-PDDF mode.

pddf_util.py imports this module and calls:

    switch-pddf     check_pddf_support, stop_platform_svc,  start_platform_pddf
    switch-nonpddf                      stop_platform_pddf, start_platform_svc

It creates the pddf_support marker between the stop and the start hook, and
starts pmon.service after the start hook returns. Everything else here is
internal.
"""
import os
import sys
import subprocess
import time
import shutil
import base64

DEVICE_DIR = "/usr/share/sonic/device/x86_64-dellemc_s5232f_c3538-r0"
PLATFORM_DIR = "/usr/share/sonic/platform"

BSP_WHEEL = os.path.join(DEVICE_DIR, "sonic_platform-1.0-py3-none-any.whl")
BSP_WHEEL_BAK = BSP_WHEEL + ".bsp_backup"
PDDF_WHEEL = os.path.join(DEVICE_DIR, "pddf", "sonic_platform-1.0-py3-none-any.whl")
PLATFORM_WHEEL = os.path.join(PLATFORM_DIR, "sonic_platform-1.0-py3-none-any.whl")

BSP_SERVICE = "platform-modules-s5232f.service"
PDDF_SERVICE = "pddf-platform-init.service"

PDDF_SUPPORT_PATHS = (
    os.path.join(DEVICE_DIR, "pddf_support"),
    os.path.join(PLATFORM_DIR, "pddf_support"),
)

STATE_DB = "6"

PLATFORM_TABLES = ("FAN_INFO", "FAN_DRAWER_INFO", "PSU_INFO", "THERMAL_INFO",
                   "PHYSICAL_ENTITY_INFO")

FANSHOW_FIELDS = ("drawer_name", "led_status", "speed", "direction",
                  "presence", "status", "timestamp")

SAFE_DEFAULTS = dict((f, "N/A") for f in FANSHOW_FIELDS)
SAFE_DEFAULTS["speed"] = "0"
LIVE_DEFAULTS = dict(SAFE_DEFAULTS, presence="True", status="True")

PDDF_NAME_GLOBS = ("FAN_INFO|Fantray*_*", "FAN_INFO|PSU*_FAN*",
                   "FAN_INFO|PSU*_fan*")
BSP_NAME_GLOBS = ("FAN_INFO|FanTray*-Fan*", "FAN_INFO|PSU* Fan",
                  "FAN_INFO|PSU* fan")

BOGUS_FAN_NAMES = ("PSU1", "PSU2", "PSU3", "PSU4", "Fan")


def check_pddf_support():
    """Required by pddf_util.py. PDDF is supported on this platform."""
    return True


def _run(cmd):
    return subprocess.getstatusoutput(cmd)


# --------------------------------------------------------------------------
# STATE_DB access
# --------------------------------------------------------------------------

def _redis(args, timeout=15):
    """Run redis-cli with an argv list (no shell) -> (rc, stdout)."""
    try:
        proc = subprocess.run(["redis-cli", "-n", STATE_DB] + args,
                              stdout=subprocess.PIPE,
                              stderr=subprocess.DEVNULL,
                              timeout=timeout)
        return proc.returncode, proc.stdout.decode("utf-8", "replace")
    except Exception:
        return 1, ""


def _redis_keys(pattern):
    rc, out = _redis(["keys", pattern])
    return [k for k in out.splitlines() if k.strip()] if rc == 0 else []


def _redis_delete(keys):
    """Delete keys, tolerating names that contain spaces."""
    keys = [k for k in keys if k]
    if not keys:
        return 0
    rc, _ = _redis(["del"] + keys)
    return len(keys) if rc == 0 else 0


# --------------------------------------------------------------------------
# sonic_platform wheel
# --------------------------------------------------------------------------

def backup_bsp_wheel():
    """Backup the BSP sonic_platform wheel before switching to PDDF."""
    if os.path.exists(BSP_WHEEL):
        try:
            shutil.copy2(BSP_WHEEL, BSP_WHEEL_BAK)
            print("BSP wheel backed up to %s" % BSP_WHEEL_BAK)
        except Exception as e:
            print("Warning: failed to backup BSP wheel: %s" % e)
    else:
        print("Warning: BSP wheel not found at %s; no backup created" % BSP_WHEEL)


def restore_bsp_wheel():
    """Restore BSP wheel from backup if main file is missing."""
    if not os.path.exists(BSP_WHEEL) and os.path.exists(BSP_WHEEL_BAK):
        try:
            shutil.copy2(BSP_WHEEL_BAK, BSP_WHEEL)
            print("BSP wheel restored from backup")
        except Exception as e:
            print("Warning: failed to restore BSP wheel: %s" % e)


def copy_wheel_for_pmon(wheel_path):
    """Ensure the selected wheel is available at /usr/share/sonic/platform/"""
    if not os.path.exists(wheel_path):
        print("Warning: wheel not found at %s; pmon may fail to load sonic_platform" % wheel_path)
        return
    try:
        if not os.path.isdir(PLATFORM_DIR):
            os.makedirs(PLATFORM_DIR, exist_ok=True)
        # realpath resolves symlinks (/usr/share/sonic/platform -> device dir)
        # so we don't attempt to copy a file onto itself.
        if os.path.realpath(wheel_path) == os.path.realpath(PLATFORM_WHEEL):
            print("Wheel already staged at %s" % PLATFORM_WHEEL)
            return
        shutil.copy2(wheel_path, PLATFORM_WHEEL)
        print("Wheel %s placed at %s for pmon" % (wheel_path, PLATFORM_WHEEL))
    except Exception as e:
        print("Warning: failed to copy wheel for pmon: %s" % e)


# --------------------------------------------------------------------------
# STATE_DB platform tables
# --------------------------------------------------------------------------

def clear_stale_platform_data():
    """Drop every platform table so the new mode starts from a clean slate."""
    total = 0
    for table in PLATFORM_TABLES:
        total += _redis_delete(_redis_keys(table + "|*"))
    print("Cleared %d stale platform entries from STATE_DB" % total)


# --------------------------------------------------------------------------
# Kernel modules and I2C topology
# --------------------------------------------------------------------------

def i2c_adapter_count():
    """Return the number of I2C adapters currently registered."""
    for path in ("/sys/class/i2c-dev", "/sys/bus/i2c/devices"):
        rc, out = _run("ls -1d %s/i2c-* 2>/dev/null | wc -l" % path)
        if rc != 0:
            continue
        try:
            count = int(out.strip())
        except (TypeError, ValueError):
            continue
        if count > 0:
            return count
    return -1


def log_i2c_state(tag):
    """Print the current I2C adapter count with a tag for before/after tracing."""
    print("[i2c] %s: %d adapters" % (tag, i2c_adapter_count()))


def unload_ipmi_modules():
    """Unload IPMI modules in the verified order before PDDF module removal."""
    for mod in ("ipmi_ssif", "acpi_ipmi"):
        rc, _ = _run("modprobe -r %s 2>/dev/null" % mod)
        if rc == 0:
            print("Unloaded IPMI module: %s" % mod)

    _run("echo 0 > /sys/module/ipmi_si/parameters/kipmid_max_busy_us 2>/dev/null")

    for mod in ("ipmi_si", "ipmi_msghandler"):
        rc, _ = _run("modprobe -r %s 2>/dev/null" % mod)
        if rc != 0:
            time.sleep(0.5)
            rc, _ = _run("modprobe -r %s 2>/dev/null" % mod)
        if rc == 0:
            print("Unloaded IPMI module: %s" % mod)
        else:
            print("Warning: could not unload IPMI module: %s" % mod)

    print("IPMI teardown complete")


def unload_pddf_modules_ordered():
    """Unload PDDF modules in correct dependency order."""
    pddf_modules_ordered = [
        "pddf_fpgapci_module",
        "pddf_xilinx_device_7021_algo",
        "pddf_xcvr_driver_module",
        "pddf_xcvr_module",
        "pddf_led_module",
        "pddf_mux_module",
        "pddf_fpgapci_driver",
        "pddf_multifpgapci_driver",
        "pddf_cpld_driver",
        "pddf_fpgai2c_driver",
        "pddf_fpgai2c_module",
        "pddf_cpld_module",
        "pddf_client_module"
    ]

    unloaded = []
    failed = []

    for mod in pddf_modules_ordered:
        rc, _ = _run("modprobe -r %s 2>/dev/null" % mod)
        if rc == 0:
            unloaded.append(mod)
        else:
            rc_check, _ = _run("lsmod | grep -q '^%s '" % mod)
            if rc_check == 0:
                failed.append(mod)

    if unloaded:
        print("Unloaded PDDF modules: %s" % ", ".join(unloaded))
    if failed:
        print("Warning: Failed to unload PDDF modules: %s" % ", ".join(failed))

    return len(failed) == 0


def force_unload_pddf_modules():
    """Force unload all PDDF kernel modules using improved sequence."""
    unload_ipmi_modules()

    success = unload_pddf_modules_ordered()

    if not success:
        status, out = _run("lsmod | awk '/^pddf/{print $1}'")
        if status == 0 and out.strip():
            mods = [m.strip() for m in out.strip().splitlines() if m.strip()]
            for _ in range(3):
                progress = False
                for m in list(mods):
                    rc, _ = _run("modprobe -rf %s 2>/dev/null" % m)
                    if rc == 0:
                        mods.remove(m)
                        progress = True
                if not mods:
                    break
                if not progress:
                    time.sleep(0.5)
            if mods:
                print("Warning: some PDDF modules still loaded: %s" % mods)

    print("PDDF module unload complete")


def unload_bsp_fpga_modules():
    """Remove BSP-specific FPGA modules that conflict with PDDF drivers.

    systemd-modules-load loads dell_s5232f_fpga_ocores at boot (from
    /etc/modules-load.d/).  If it is still loaded when PDDF drivers start,
    both drivers compete for the FPGA PCI device, causing 'Bad IO access'
    kernel warnings and xcvr presence detection failures on all ports.

    Must be called BEFORE 'systemctl start pddf-platform-init.service'.
    """
    for mod in ("dell_s5232f_fpga_ocores", "i2c_ocores"):
        rc, _ = _run("lsmod | grep -q '^%s '" % mod)
        if rc != 0:
            continue  # Module not loaded
        rc, _ = _run("modprobe -r %s 2>/dev/null" % mod)
        if rc != 0:
            time.sleep(0.5)
            rc, _ = _run("modprobe -r %s 2>/dev/null" % mod)
        if rc == 0:
            print("Unloaded BSP FPGA module: %s" % mod)
        else:
            print("Warning: could not unload BSP FPGA module: %s" % mod)


def reload_bsp_fpga_modules():
    """Reload BSP FPGA modules when switching back to Non-PDDF.

    dell_s5232f_fpga_ocores and i2c_ocores are removed by
    pddf_pre_driver_install.sh when entering PDDF mode.  They must be
    restored for the BSP platform service to create its full I2C topology
    (contributes to the 41 vs 50 adapter-count difference).
    """
    for mod in ("i2c_ocores", "dell_s5232f_fpga_ocores"):
        rc, _ = _run("modprobe %s 2>/dev/null" % mod)
        if rc == 0:
            print("Reloaded BSP FPGA module: %s" % mod)
        else:
            print("Warning: could not reload BSP FPGA module: %s" % mod)


def restore_bsp_i2c_providers():
    """Reload the I2C providers that pddf_pre_driver_install.sh removed."""
    for mod in ("i2c_i801", "i2c_ismt"):
        rc, _ = _run("modprobe %s 2>/dev/null" % mod)
        if rc == 0:
            print("Reloaded I2C provider: %s" % mod)
        else:
            print("Warning: could not reload I2C provider: %s" % mod)


# --------------------------------------------------------------------------
# pddf_support marker
# --------------------------------------------------------------------------

def create_pddf_support_file():
    """Create the pddf_support marker file (idempotent safety net)."""
    ok = False
    for path in PDDF_SUPPORT_PATHS:
        parent = os.path.dirname(path)
        if not os.path.isdir(parent):
            continue
        try:
            with open(path, "a"):
                pass
            print("Created pddf_support marker: %s" % path)
            ok = True
        except Exception as e:
            print("Warning: failed to create %s: %s" % (path, e))
    if not ok:
        print("Warning: no pddf_support marker could be created")
    return ok


def remove_pddf_support_file():
    """Remove the pddf_support marker file(s) when leaving PDDF mode."""
    for path in PDDF_SUPPORT_PATHS:
        try:
            if os.path.exists(path) or os.path.islink(path):
                os.remove(path)
                print("Removed pddf_support marker: %s" % path)
        except Exception as e:
            print("Warning: failed to remove %s: %s" % (path, e))
    return True


# --------------------------------------------------------------------------
# pmon
# --------------------------------------------------------------------------

def remove_pmon_container():
    """Remove pmon container so it is recreated with the correct I2C device list."""
    _run("systemctl reset-failed pmon.service 2>/dev/null")

    rc, _ = _run("docker rm -f pmon 2>/dev/null")
    if rc == 0:
        print("Removed pmon container for device list refresh")


def _docker_exec_py_in_pmon(py_code):
    """Execute python code inside the pmon container reliably."""
    try:
        b64 = base64.b64encode(py_code.encode("utf-8")).decode("ascii")
        cmd = ("docker exec pmon python3 -c "
               "'import base64;exec(base64.b64decode(\"%s\"))' 2>&1" % b64)
        return _run(cmd)
    except Exception as e:
        return 1, str(e)


def _get_live_fan_names_from_pmon():
    """Authoritative fan names from sonic_platform inside pmon."""
    py = r'''
from sonic_platform.chassis import Chassis
names = set()
try:
    c = Chassis()
    for i in range(getattr(c, "get_num_fans", lambda:0)()):
        try: names.add(c.get_fan(i).get_name())
        except: pass
    for i in range(getattr(c, "get_num_psus", lambda:0)()):
        try:
            for f in (c.get_psu(i).get_all_fans() or []):
                names.add(f.get_name())
        except: pass
except Exception:
    pass
print("LIVENAMES:" + ",".join(sorted(names)))
'''
    rc, out = _docker_exec_py_in_pmon(py)
    if rc != 0 or "LIVENAMES:" not in out:
        return set()
    tail = out.split("LIVENAMES:", 1)[1].strip()
    return set([x for x in tail.split(",") if x])


# --------------------------------------------------------------------------
# FAN_INFO reconciliation
# --------------------------------------------------------------------------

def in_pddf_mode():
    return os.path.exists(os.path.join(PLATFORM_DIR, "pddf_support"))


def reconcile_fan_name_families():
    """Keep only the fan-name family belonging to the current mode."""
    _redis_delete(["FAN_INFO|" + name for name in BOGUS_FAN_NAMES])
    for glob in (BSP_NAME_GLOBS if in_pddf_mode() else PDDF_NAME_GLOBS):
        _redis_delete(_redis_keys(glob))


def _ensure_fan_fields(keys, defaults):
    """Fill in only the missing fields."""
    for key in keys:
        rc, out = _redis(["hgetall", key])
        if rc != 0:
            continue
        present = set(out.splitlines()[0::2])
        missing = []
        for field in FANSHOW_FIELDS:
            if field not in present:
                missing += [field, defaults[field]]
        if missing:
            _redis(["hset", key] + missing)


def repair_fan_info_for_show():
    """Make FAN_INFO consistent with the current mode and safe for fanshow."""
    reconcile_fan_name_families()

    live = _get_live_fan_names_from_pmon()
    if live:
        stale = [k for k in _redis_keys("FAN_INFO|*")
                 if k.split("|", 1)[-1] not in live]
        _redis_delete(stale)
        _ensure_fan_fields(["FAN_INFO|" + name for name in live], LIVE_DEFAULTS)

    _ensure_fan_fields(_redis_keys("FAN_INFO|*"), SAFE_DEFAULTS)


def start_pmon_and_repair():
    """Start pmon, wait for it to populate FAN_INFO, then repair safely.

    The container is removed and recreated so that Trixie unprivileged mode
    picks up the correct I2C ``--device`` flags for the current mode.
    ``systemctl reset-failed`` clears any previous start-limit-burst so
    the subsequent ``systemctl start`` is not rejected.
    """
    remove_pmon_container()
    _run("systemctl start pmon.service 2>/dev/null")
    time.sleep(15)
    try:
        repair_fan_info_for_show()
        print("FAN_INFO repair completed")
    except Exception as e:
        print("FAN_INFO repair failed: %s" % e)


# --------------------------------------------------------------------------
# Hooks called by pddf_util.py
# --------------------------------------------------------------------------

def stop_platform_svc():
    """Prepare for switch to PDDF: backup BSP wheel, drop pmon, clear state, stop BSP service."""
    log_i2c_state("stop_platform_svc: entry (Non-PDDF)")

    backup_bsp_wheel()
    remove_pmon_container()
    clear_stale_platform_data()

    status, _ = _run("systemctl stop %s" % BSP_SERVICE)
    if status:
        print("Stop %s failed %d" % (BSP_SERVICE, status))
    status, _ = _run("systemctl disable %s" % BSP_SERVICE)
    if status:
        print("Disable %s failed %d" % (BSP_SERVICE, status))

    _run("systemctl stop %s 2>/dev/null" % PDDF_SERVICE)

    unload_ipmi_modules()

    log_i2c_state("stop_platform_svc: exit")
    return True


def start_platform_svc():
    """Switch back to Non-PDDF: clear state, start BSP service, restore wheel, stage for pmon."""
    log_i2c_state("start_platform_svc: entry")

    clear_stale_platform_data()

    restore_bsp_i2c_providers()

    # Reload BSP FPGA modules removed when entering PDDF mode.
    # These must be present before the BSP service creates its I2C topology.
    reload_bsp_fpga_modules()

    status, _ = _run("systemctl enable %s" % BSP_SERVICE)
    if status:
        print("Enable %s failed %d" % (BSP_SERVICE, status))
        return False
    status, _ = _run("systemctl start %s" % BSP_SERVICE)
    if status:
        print("Start %s failed %d" % (BSP_SERVICE, status))
        return False

    restore_bsp_wheel()
    copy_wheel_for_pmon(BSP_WHEEL)
    start_pmon_and_repair()

    log_i2c_state("start_platform_svc: exit (Non-PDDF)")
    return True


def start_platform_pddf():
    """Start PDDF mode."""
    log_i2c_state("start_platform_pddf: entry")

    # Unload BSP FPGA modules FIRST - they conflict with PDDF FPGA drivers.
    # systemd-modules-load loads dell_s5232f_fpga_ocores at every boot from
    # /etc/modules-load.d/.  If it stays resident, both BSP and PDDF drivers
    # claim the FPGA PCI BAR and xcvr presence reads hit wrong IO ports
    # (kernel: "Bad IO access at port 0x4004").
    unload_bsp_fpga_modules()

    force_unload_pddf_modules()
    time.sleep(1)

    create_pddf_support_file()

    clear_stale_platform_data()

    status, _ = _run("systemctl enable %s" % PDDF_SERVICE)
    if status:
        print("Enable %s failed %d" % (PDDF_SERVICE, status))
        return False
    status, _ = _run("systemctl start %s" % PDDF_SERVICE)
    if status:
        print("Start %s failed %d" % (PDDF_SERVICE, status))
        return False

    copy_wheel_for_pmon(PDDF_WHEEL)
    start_pmon_and_repair()

    log_i2c_state("start_platform_pddf: exit (PDDF)")
    return True


def stop_platform_pddf():
    """Stop PDDF platform service and ensure drivers are gone.

    Note: the ``pddf_util.py clean -f`` and PDDF service stop may emit
    harmless kernel messages such as ``i2c i2c-0: delete_device: Can't
    find device in list`` when devices were already removed by the service
    stop; these are safe to ignore.
    """
    log_i2c_state("stop_platform_pddf: entry (PDDF)")

    remove_pmon_container()
    clear_stale_platform_data()

    status, _ = _run("systemctl stop %s" % PDDF_SERVICE)
    if status:
        print("Stop %s failed %d" % (PDDF_SERVICE, status))
    status, _ = _run("systemctl disable %s" % PDDF_SERVICE)
    if status:
        print("Disable %s failed %d" % (PDDF_SERVICE, status))

    # Remove marker BEFORE pddf_util.py clean so do_uninstall() returns early.
    # Without this, clean deletes the BSP sonic_platform wheel from the device dir.
    remove_pddf_support_file()

    _run("/usr/local/bin/pddf_util.py clean -f 2>/dev/null")

    force_unload_pddf_modules()

    log_i2c_state("stop_platform_pddf: exit")
    return True

