#!/usr/bin/env python3
"""Dell S5224F hooks for switching between PDDF and Non-PDDF mode.

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

DEVICE_DIR = "/usr/share/sonic/device/x86_64-dellemc_s5224f_c3538-r0"
PLATFORM_DIR = "/usr/share/sonic/platform"

BSP_WHEEL = os.path.join(DEVICE_DIR, "sonic_platform-1.0-py3-none-any.whl")
BSP_WHEEL_BAK = BSP_WHEEL + ".bsp_backup"
PDDF_WHEEL = os.path.join(DEVICE_DIR, "pddf", "sonic_platform-1.0-py3-none-any.whl")
PLATFORM_WHEEL = os.path.join(PLATFORM_DIR, "sonic_platform-1.0-py3-none-any.whl")

BSP_SERVICE = "platform-modules-s5224f.service"
PDDF_SERVICE = "pddf-platform-init.service"

# pddf_util.py checks PLATFORM_DIR while the manual procedure touches DEVICE_DIR.
# PLATFORM_DIR is normally a symlink to DEVICE_DIR, but handle both so the marker
# is still correct if the symlink is missing.
PDDF_SUPPORT_PATHS = (
    os.path.join(DEVICE_DIR, "pddf_support"),
    os.path.join(PLATFORM_DIR, "pddf_support"),
)

STATE_DB = "6"

# Tables pmon repopulates once it restarts in the new mode.
PLATFORM_TABLES = ("FAN_INFO", "FAN_DRAWER_INFO", "PSU_INFO", "THERMAL_INFO",
                   "PHYSICAL_ENTITY_INFO")

# Fields fanshow dereferences without a default; a missing one is a KeyError.
FANSHOW_FIELDS = ("drawer_name", "led_status", "speed", "direction",
                  "presence", "status", "timestamp")

# Placeholders only bridge the seconds before psud/thermalctld write real values.
SAFE_DEFAULTS = dict((f, "N/A") for f in FANSHOW_FIELDS)
SAFE_DEFAULTS["speed"] = "0"
# For names the platform API just reported, presence/status are known good.
LIVE_DEFAULTS = dict(SAFE_DEFAULTS, presence="True", status="True")

# The two fan-name families. PDDF calls them 'Fantray3_2' and 'PSU1_FAN1'; the
# BSP calls the same hardware 'FanTray3-Fan2' and 'PSU1 Fan'. redis KEYS globs
# are case sensitive, which is what keeps these two sets disjoint.
PDDF_NAME_GLOBS = ("FAN_INFO|Fantray*_*", "FAN_INFO|PSU*_FAN*",
                   "FAN_INFO|PSU*_fan*")
BSP_NAME_GLOBS = ("FAN_INFO|FanTray*-Fan*", "FAN_INFO|PSU* Fan",
                  "FAN_INFO|PSU* fan")

# Names psud sometimes emits that are never valid fans.
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
    """Run redis-cli with an argv list (no shell) -> (rc, stdout).

    Passing argv is what makes keys containing spaces work. Do not go back to
    'redis-cli keys ... | xargs redis-cli del': xargs splits on whitespace, so
    'FAN_INFO|PSU1 Fan' became the two arguments 'FAN_INFO|PSU1' and 'Fan'. The
    real key was never deleted, and deleting the fragments is what created the
    phantom bare 'PSU1' / 'Fan' keys.
    """
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
    """Backup the BSP sonic_platform wheel before switching to PDDF.

    If we don't back it up, pddf_util.py's cleanup_pddf_utils may delete it
    (when it sees PDDF wheel present and no .orig backup), and subsequent
    Non-PDDF boots/switches will fail to install sonic_platform.
    """
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
    """Ensure the selected wheel is available at /usr/share/sonic/platform/

    docker_init.j2 inside pmon looks for sonic_platform wheel at that path
    and will pip install it if sonic_platform is not importable. We must
    place the correct (BSP or PDDF) wheel there before starting pmon.
    """
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
#
# 'show platform fan' (fanshow) does bare dict access on FANSHOW_FIELDS, so any
# entry missing one of them raises KeyError. Two things create such entries
# around a switch: cross-family names left over from the previous mode, and
# entries half-written when psud/thermalctld were stopped mid-update.
# --------------------------------------------------------------------------

def clear_stale_platform_data():
    """Drop every platform table so the new mode starts from a clean slate.

    After a switch these tables still hold the other mode's naming, and fanshow
    crashes on any entry that is missing a field. The start paths repopulate
    them from the live platform once pmon is up.
    """
    total = 0
    for table in PLATFORM_TABLES:
        total += _redis_delete(_redis_keys(table + "|*"))
    print("Cleared %d stale platform entries from STATE_DB" % total)


# --------------------------------------------------------------------------
# Kernel modules and I2C topology
# --------------------------------------------------------------------------

def i2c_adapter_count():
    """Return the number of I2C adapters currently registered.

    Equivalent to 'i2cdetect -l | wc -l'. Used to detect the topology
    discrepancy seen between PDDF (41) and Non-PDDF (50) modes.
    """
    # /sys/class/i2c-adapter does not exist on this kernel; /sys/class/i2c-dev
    # matches 'i2cdetect -l' exactly. Fall back to the bus view if absent.
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
    """Unload IPMI modules in the verified order before PDDF module removal.

    'modprobe -r ipmi_si' fails while the kipmid kernel thread is busy-polling.
    s5224f_platform.sh init loads it with kipmid_max_busy_us=1000, so kipmid must
    be quiesced first, otherwise ipmi_si stays resident, keeps its SMBus/ACPI
    references and skews the I2C adapter topology across a mode switch.
    """
    for mod in ("ipmi_ssif", "acpi_ipmi"):
        rc, _ = _run("modprobe -r %s 2>/dev/null" % mod)
        if rc == 0:
            print("Unloaded IPMI module: %s" % mod)

    # Quiesce kipmid immediately before removing ipmi_si.
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
    """Unload PDDF modules in correct dependency order.

    Based on analysis showing I2C device count differences (41 in PDDF vs 50 in Non-PDDF),
    proper module unloading order is critical. Remove higher-level drivers before
    the base pddf_client_module.
    """
    # Ordered list: remove dependent modules before base modules
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
        "pddf_client_module"  # Base module - remove last
    ]

    unloaded = []
    failed = []

    for mod in pddf_modules_ordered:
        rc, _ = _run("modprobe -r %s 2>/dev/null" % mod)
        if rc == 0:
            unloaded.append(mod)
        else:
            # Check if module exists but failed to unload
            rc_check, _ = _run("lsmod | grep -q '^%s '" % mod)
            if rc_check == 0:
                failed.append(mod)

    if unloaded:
        print("Unloaded PDDF modules: %s" % ", ".join(unloaded))
    if failed:
        print("Warning: Failed to unload PDDF modules: %s" % ", ".join(failed))

    return len(failed) == 0


def force_unload_pddf_modules():
    """Force unload all PDDF kernel modules using improved sequence.

    Combines IPMI removal and ordered PDDF module unloading for clean transitions.
    This addresses I2C device topology changes between modes.
    """
    # Step 1: Unload IPMI modules first
    unload_ipmi_modules()

    # Step 2: Unload PDDF modules in dependency order
    success = unload_pddf_modules_ordered()

    # Step 3: Fallback - try generic unload for any remaining pddf modules
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

    systemd-modules-load loads dell_s5224f_fpga_ocores at boot (from
    /etc/modules-load.d/).  If it is still loaded when PDDF drivers start,
    both drivers compete for the FPGA PCI device, causing 'Bad IO access'
    kernel warnings and xcvr presence detection failures on all ports.

    Must be called BEFORE 'systemctl start pddf-platform-init.service'.
    """
    for mod in ("dell_s5224f_fpga_ocores", "i2c_ocores"):
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

    dell_s5224f_fpga_ocores and i2c_ocores are removed by
    pddf_pre_driver_install.sh when entering PDDF mode.  They must be
    restored for the BSP platform service to create its full I2C topology
    (contributes to the 41 vs 50 adapter-count difference).
    """
    for mod in ("i2c_ocores", "dell_s5224f_fpga_ocores"):
        rc, _ = _run("modprobe %s 2>/dev/null" % mod)
        if rc == 0:
            print("Reloaded BSP FPGA module: %s" % mod)
        else:
            print("Warning: could not reload BSP FPGA module: %s" % mod)


def restore_bsp_i2c_providers():
    """Reload the I2C providers that pddf_pre_driver_install.sh removed.

    pddf_pre_driver_install.sh unloads dell_s5224f_fpga_ocores, i2c-i801 and
    i2c_ismt before PDDF drivers are installed. s5224f_platform.sh 'init' only
    reloads i2c_ocores/dell_s5224f_fpga_ocores, so i2c-i801 and i2c_ismt never
    come back after PDDF -> Non-PDDF. Their adapters are then permanently
    missing, which is a direct contributor to the adapter-count discrepancy.
    """
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
    """Create the pddf_support marker file (idempotent safety net).

    pddf_util.py normally creates this between stop_platform_svc() and
    start_platform_pddf(); we re-assert it so start_platform_pddf() is also
    correct when invoked standalone.
    """
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
    """Remove pmon container so it is recreated with the correct I2C device list.

    In Trixie unprivileged mode, pmon has static --device flags baked at
    container creation. After mode switch the I2C topology changes; the
    old container will fail or see wrong devices. Removing it forces
    systemd to recreate it on next start.
    """
    # Removing the container underneath a running unit makes pmon.service exit
    # repeatedly and trip systemd's StartLimitBurst, after which the subsequent
    # 'systemctl start pmon.service' fails with 'start-limit-hit'. Clear the
    # failure counter so the later start is allowed.
    _run("systemctl reset-failed pmon.service 2>/dev/null")

    rc, _ = _run("docker rm -f pmon 2>/dev/null")
    if rc == 0:
        print("Removed pmon container for device list refresh")


def _docker_exec_py_in_pmon(py_code):
    """Execute python code inside the pmon container reliably.

    Do NOT use 'docker cp ... pmon:/tmp/...': pmon has a mount over /tmp, so the
    copy lands in the image layer and is invisible to the running container.
    'docker cp' still reports success, so every call silently failed with
    "python3: can't open file '/tmp/_pmon_repair.py'".

    Instead base64-encode the payload into the command line. This needs no
    shared filesystem and is immune to shell quoting of the embedded source.
    Returns (rc, stdout+stderr).
    """
    try:
        b64 = base64.b64encode(py_code.encode("utf-8")).decode("ascii")
        cmd = ("docker exec pmon python3 -c "
               "'import base64;exec(base64.b64decode(\"%s\"))' 2>&1" % b64)
        return _run(cmd)
    except Exception as e:
        return 1, str(e)


def _get_live_fan_names_from_pmon():
    """Authoritative fan names from sonic_platform inside pmon.

    Includes PSU fans: chassis.get_all_fans() returns only tray fans, so PSU
    fans have to be collected separately or they look stale and get deleted.
    Returns an empty set while pmon is still starting.
    """
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
    """Keep only the fan-name family belonging to the current mode.

    Must stay mode-aware. In Non-PDDF 'PSU1 Fan' is the *legitimate* name, so a
    blanket 'FAN_INFO|* Fan*' delete discards live psud data and makes
    'show platform fan' flap to Not Present until psud rewrites it.
    """
    _redis_delete(["FAN_INFO|" + name for name in BOGUS_FAN_NAMES])
    for glob in (BSP_NAME_GLOBS if in_pddf_mode() else PDDF_NAME_GLOBS):
        _redis_delete(_redis_keys(glob))


def _ensure_fan_fields(keys, defaults):
    """Fill in only the missing fields.

    Never overwrite a value psud or thermalctld already wrote, otherwise the
    repair clobbers live readings with placeholders.
    """
    for key in keys:
        rc, out = _redis(["hgetall", key])
        if rc != 0:
            continue
        # hgetall returns field, value, field, value ...
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
        # Anything the platform no longer reports is stale by definition, which
        # also covers cross-family leftovers the globs did not match.
        stale = [k for k in _redis_keys("FAN_INFO|*")
                 if k.split("|", 1)[-1] not in live]
        _redis_delete(stale)
        _ensure_fan_fields(["FAN_INFO|" + name for name in live], LIVE_DEFAULTS)

    # Whatever remains, including writes that landed after the snapshot above,
    # still must not crash fanshow.
    _ensure_fan_fields(_redis_keys("FAN_INFO|*"), SAFE_DEFAULTS)


# --------------------------------------------------------------------------
# Hooks called by pddf_util.py
# --------------------------------------------------------------------------

def stop_platform_svc():
    """Prepare for switch to PDDF: backup BSP wheel, drop pmon, clear state, stop BSP service.

    Note: do NOT touch the pddf_support marker here. pddf_util.do_switch_pddf()
    calls this while switching *to* PDDF and creates the marker immediately after.
    """
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

    # s5224f_platform.sh deinit cannot remove ipmi_si while kipmid is busy.
    # Tear IPMI down properly so it does not leak into PDDF mode.
    unload_ipmi_modules()

    log_i2c_state("stop_platform_svc: exit")
    return True


def start_platform_svc():
    """Switch back to Non-PDDF: clear state, start BSP service, restore wheel, stage for pmon."""
    log_i2c_state("start_platform_svc: entry")

    clear_stale_platform_data()

    # pddf_pre_driver_install.sh removed i2c-i801/i2c_ismt on the way into PDDF
    # and nothing reloads them. Do it before the BSP service instantiates its
    # muxes/devices, otherwise those adapters stay missing in Non-PDDF mode.
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

    # Remove pmon container so it is recreated with the correct I2C device list.
    # pddf_util.py starts pmon.service right after this returns.
    remove_pmon_container()

    repair_fan_info_for_show()

    log_i2c_state("start_platform_svc: exit (Non-PDDF)")
    return True


def start_platform_pddf():
    """Start PDDF mode.

    IMPORTANT: Do NOT call 'pddf_util.py clean -f' here. That path eventually
    calls cleanup_pddf_utils which, when pddf_support exists and no .orig
    backup is present, can delete the BSP wheel file from the device directory.
    Instead, just force-unload any stale PDDF modules directly.
    """
    log_i2c_state("start_platform_pddf: entry")

    # Unload BSP FPGA modules FIRST - they conflict with PDDF FPGA drivers.
    # systemd-modules-load loads dell_s5224f_fpga_ocores at every boot from
    # /etc/modules-load.d/.  If it stays resident, both BSP and PDDF drivers
    # claim the FPGA PCI BAR and xcvr presence reads hit wrong IO ports
    # (kernel: "Bad IO access at port 0x4004").
    unload_bsp_fpga_modules()

    # Unload stale PDDF and IPMI modules in proper order
    force_unload_pddf_modules()
    time.sleep(1)

    # Re-assert the pddf_support marker (pddf_util normally creates it first)
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

    # Remove pmon container so it is recreated with the correct I2C device list.
    # pddf_util.py starts pmon.service right after this returns.
    remove_pmon_container()

    repair_fan_info_for_show()

    log_i2c_state("start_platform_pddf: exit (PDDF)")
    return True


def stop_platform_pddf():
    """Stop PDDF platform service and ensure drivers are gone.

    Properly unload IPMI and PDDF modules in correct order, then remove
    pddf_support marker file for clean transition to Non-PDDF mode.

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
