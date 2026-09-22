# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Dependency-aware kernel module reload.

Backs the `nh_module` CLI. Parses /proc/modules to find a module's dependents
automatically and unloads/reloads them in the correct order; PDDF-managed
modules go through a full `pddf_util.py` teardown and reinstall.
"""

import os
import re
from contextlib import contextmanager
from subprocess import CalledProcessError

from nexthop.pddf_loader import load_pddf_device_config
from nexthop_utils.platform_utils import run_cmd
from sonic_py_common import syslogger

PROC_MODULES_PATH = "/proc/modules"
PDDF_UTIL_PATH = "/usr/local/bin/pddf_util.py"
MODULE_NAME_RE = re.compile(r"^[A-Za-z0-9_-]+$")
I2C_DRIVERS_PATH = "/sys/bus/i2c/drivers"
MULTIFPGAPCI_DEVICES_PATH = "/sys/kernel/pddf/devices/multifpgapci"
I2C_CLIENT_RE = re.compile(r"^\d+-[0-9a-f]+$")
PRINTK_PATH = "/proc/sys/kernel/printk"
# Console loglevel that keeps KERN_ERR (3) off the console while still recording
# it in the kernel ring buffer, so `dmesg` after a reload loses nothing.
QUIET_CONSOLE_LOGLEVEL = 1
# How long the FPGA watchdog stays armed while a reload runs, with punching
# paused for the same duration so nothing shortens the counter. Must exceed the
# longest legitimate teardown (over 300s has been observed); a reload stuck past
# it resets the switch, which is the guardrail for a wedged or killed reload.
RELOAD_WATCHDOG_SECONDS = 900
# Upper bound of the 24-bit millisecond counter, in whole seconds.
MAX_WATCHDOG_SECONDS = 0xFFFFFF // 1000
# Mirrors sonic_platform.watchdog._WATCHDOG_PAUSE_FILE_PATH, which cannot be
# imported here (see below), and needed as a fallback when the teardown has
# uninstalled the wheel that `watchdogutil disarm` would import.
WATCHDOG_PAUSE_FILE_PATH = "/var/lock/pddf-locks/watchdog.pause"
# The watchdog is driven through the standard `watchdogutil` CLI rather than by
# importing sonic_platform, for two reasons. `sonic_platform/__init__.py`
# star-imports the whole platform API (chassis, sfp, psu, thermal, ...), and those
# modules import back into this package - fpga_lib, xcvr_presence_cache,
# mgmt_port_link_state - so importing it here inverts the dependency direction for
# a CLI that otherwise only needs modprobe. It also reads the very stack this
# reload tears down, so each call wants a fresh process rather than objects held
# across the teardown.
WATCHDOGUTIL = "watchdogutil"
# watchdogutil exits with this status when it cannot build the platform watchdog:
# the platform declares no WATCHDOG (cf2 standalone), or the sonic_platform wheel
# is not installed. Its arm/disarm/status subcommands exit 0 even on hardware
# failure and report the result on stdout, hence the success markers below.
WATCHDOGUTIL_LOAD_ERROR_STATUS = 2


_logger = syslogger.SysLogger("nexthop.module_reload_lib")


class ModuleReloadError(Exception):
    """Raised when a module unload/reload/device-teardown step fails."""


def _normalize(module_name):
    """Kernel module names in /proc/modules always use underscores."""
    return module_name.replace("-", "_")


def _validate_module_name(module_name):
    """Reject anything that isn't a plain module name.

    module_name is interpolated into a command string that run_cmd() later
    splits with shlex; this keeps malformed input (stray spaces, shell
    metacharacters) from being misparsed into unintended arguments.
    """
    if not MODULE_NAME_RE.match(module_name):
        raise ModuleReloadError(
            f"Invalid module name '{module_name}': must match {MODULE_NAME_RE.pattern}"
        )


def _run_cmd_or_raise(cmd, error_prefix):
    try:
        run_cmd(cmd)
    except Exception as e:
        message = f"{error_prefix}: {e}"
        # Also syslogged: a CI harness may capture only the CLI's stdout, and
        # the DUT's own record of what failed must not depend on that.
        _logger.log_error(message)
        raise ModuleReloadError(message) from e


def get_loaded_modules():
    """Return {module_name: [direct dependent module names]} from /proc/modules.

    The 4th whitespace-separated field of /proc/modules is the same
    comma-separated "Used by" list that `lsmod` displays: the modules
    currently depending on (using) this module, not the modules it depends on.
    """
    modules = {}
    with open(PROC_MODULES_PATH, "r") as f:
        lines = f.readlines()

    for line in lines:
        fields = line.split()
        if not fields:
            continue
        name = fields[0]
        dependents_field = fields[3] if len(fields) > 3 else "-"
        if dependents_field == "-":
            dependents = []
        else:
            dependents = [d for d in dependents_field.rstrip(",").split(",") if d]
        modules[name] = dependents

    return modules


def is_module_loaded(module_name, loaded_modules=None):
    loaded_modules = loaded_modules if loaded_modules is not None else get_loaded_modules()
    return _normalize(module_name) in loaded_modules


def get_dependents(module_name, loaded_modules=None):
    """Direct dependents (modules currently using module_name)."""
    loaded_modules = loaded_modules if loaded_modules is not None else get_loaded_modules()
    name = _normalize(module_name)
    if name not in loaded_modules:
        raise ModuleReloadError(f"Module '{module_name}' is not loaded")
    return loaded_modules[name]


def _unload_order_for(module_names, loaded_modules):
    """Combined unload order for every loaded module in module_names.

    Each module is unloaded only after all of its (transitive) dependents,
    including dependents shared between more than one module in module_names.
    """
    order = []
    visited = set()

    def visit(mod):
        if mod in visited:
            return
        visited.add(mod)
        for dependent in loaded_modules.get(mod, []):
            visit(dependent)
        order.append(mod)

    for module_name in module_names:
        name = _normalize(module_name)
        if name in loaded_modules:
            visit(name)

    return order


def get_unload_order(module_name, loaded_modules=None):
    """Modules in the order they must be unloaded to remove `module_name`.

    Returns a list ending in `module_name` itself, with every (transitive)
    dependent appearing before it, deepest dependent first.
    """
    loaded_modules = loaded_modules if loaded_modules is not None else get_loaded_modules()
    name = _normalize(module_name)
    if name not in loaded_modules:
        raise ModuleReloadError(f"Module '{module_name}' is not loaded")

    return _unload_order_for([name], loaded_modules)


def get_declared_modules():
    """Return (pddf_kos, custom_kos) declared for the current platform, or ([], []) if unavailable."""
    try:
        config = load_pddf_device_config()
    except (FileNotFoundError, OSError):
        return [], []
    platform = config.get("PLATFORM", {})
    return platform.get("pddf_kos", []), platform.get("custom_kos", [])


def is_pddf_module(module_name):
    """Whether module_name is declared in the current platform's pddf-device.json."""
    pddf_kos, custom_kos = get_declared_modules()
    declared = {_normalize(m) for m in (set(pddf_kos) | set(custom_kos))}
    return _normalize(module_name) in declared


def _run_watchdogutil(subcommand, success_marker):
    """Run `watchdogutil <subcommand>`. True if it took effect, False on failure,
    None if no platform watchdog could be built.

    Failure is reported rather than raised: platforms without a watchdog must
    still reload, and a failed re-arm self-heals once `watchdog.timer` resumes.
    """
    try:
        output = run_cmd(f"{WATCHDOGUTIL} {subcommand}")
    except CalledProcessError as e:
        if e.returncode == WATCHDOGUTIL_LOAD_ERROR_STATUS:
            _logger.log_info(
                f"no platform watchdog, skipping `watchdogutil {subcommand}`"
            )
            return None
        _logger.log_warning(
            f"`watchdogutil {subcommand}` did not take effect, continuing "
            f"anyway: {e} (stderr: {e.stderr})"
        )
        return False
    except Exception as e:
        _logger.log_warning(
            f"`watchdogutil {subcommand}` did not take effect, continuing anyway: {e}"
        )
        return False
    if success_marker not in (output or ""):
        _logger.log_warning(
            f"`watchdogutil {subcommand}` did not take effect, continuing "
            f"anyway: {output}"
        )
        return False
    return True


def _rearm_punch_watchdog():
    """Start watchdog.service, which arms the watchdog for the punch interval.

    A failure is logged, not raised: watchdog.timer retries within a minute.
    """
    try:
        run_cmd("systemctl start watchdog.service")
    except Exception as e:
        _logger.log_warning(
            f"could not re-arm the punch watchdog, watchdog.timer will retry "
            f"within a minute: {e}"
        )


def _restore_watchdog():
    """Return the watchdog to normal punching and read the armed state back.

    `watchdogutil disarm` clears the reload counter and the punching pause in one
    call, so `watchdog.timer` punches again even if the re-arm below fails, which
    is what makes that failure self-healing. Starting watchdog.service replaces
    the long reload counter with the normal punching cycle immediately instead of
    leaving the switch unprotected until the next timer tick.
    """
    disarmed = _run_watchdogutil("disarm", "Watchdog disarmed successfully")
    if disarmed is not True:
        # `pddf_util.py clean` uninstalls the sonic_platform wheel and `install`
        # puts it back, so a teardown that fails in between leaves watchdogutil
        # with nothing to import. Deleting the file is all the unpause needs,
        # and it needs neither the wheel nor the FPGA.
        try:
            os.unlink(WATCHDOG_PAUSE_FILE_PATH)
        except FileNotFoundError:
            pass
        except OSError as e:
            _logger.log_error(
                f"could not clear {WATCHDOG_PAUSE_FILE_PATH}; watchdog.timer "
                f"will not punch and the switch will reset: {e}"
            )
    if disarmed is None:
        # No watchdog to re-arm, so nothing to verify and nothing lost.
        return
    _rearm_punch_watchdog()
    if _run_watchdogutil("status", "Status: Armed") is False:
        _logger.log_error(
            "FPGA watchdog is not confirmed armed after reload; the switch may "
            "be running without watchdog protection"
        )


@contextmanager
def _reload_watchdog(watchdog_seconds):
    """Arm the FPGA watchdog for the reload window; restore normal punching on exit.

    The register lives behind the FPGA, so nothing can arm or punch once the
    teardown unmaps it. `watchdogutil arm` goes through `Watchdog.arm()`, which
    pauses `watchdog.timer` and leaves a counter long enough to outlast the
    teardown - and a reload stuck past it resets the switch.
    """
    if (
        _run_watchdogutil(
            f"arm -s {watchdog_seconds}",
            f"Watchdog armed for {watchdog_seconds} seconds",
        )
        is False
    ):
        _logger.log_error(
            "FPGA watchdog could not be armed for the reload; watchdog.timer may "
            "arm a shorter counter mid-teardown and reset the switch"
        )
    try:
        yield
    finally:
        _restore_watchdog()


def _i2c_clients_bound_to(modules):
    """{module: [i2c client, ...]} for clients bound to a driver those modules own.

    The kernel records the owning module of each i2c driver, so this is exactly
    the set whose callbacks point into code an unload is about to free. An
    unreadable sysfs tree reports nothing: the teardown verifications after this
    remain as backstops, and refusing every reload on a transient would be worse.
    """
    declared = {_normalize(m) for m in modules}
    bound = {}
    try:
        drivers = os.listdir(I2C_DRIVERS_PATH)
    except OSError as e:
        _logger.log_warning(
            f"cannot scan {I2C_DRIVERS_PATH}, skipping the bound-client check: {e}"
        )
        return bound

    for driver in drivers:
        driver_dir = os.path.join(I2C_DRIVERS_PATH, driver)
        if not os.path.islink(os.path.join(driver_dir, "module")):
            # Built-in driver: no owning module, so nothing an unload can free.
            continue
        try:
            owner = os.path.basename(os.readlink(os.path.join(driver_dir, "module")))
            if _normalize(owner) not in declared:
                continue
            # sorted(): os.listdir() order is arbitrary and these names are
            # reported to a human in the error below.
            clients = sorted(e for e in os.listdir(driver_dir) if I2C_CLIENT_RE.match(e))
        except OSError as e:
            _logger.log_warning(
                f"cannot inspect i2c driver {driver}, skipping it in the "
                f"bound-client check: {e}"
            )
            continue
        if clients:
            bound[owner] = clients

    return bound


def _fpga_i2c_adapter_indices(i2c_dir):
    """Indices of the adapters `i2c_dir` currently has registered.

    An adapter is numbered `index + virt_bus` and parented to the PCI device, so
    the live ones can be read back from sysfs instead of assuming every declared
    channel was created. Deleting an index that was never registered relies on
    the kernel guarding it, which older images do not.
    """
    try:
        with open(os.path.join(i2c_dir, "num_virt_ch")) as f:
            num_virt_ch = int(f.read().strip(), 16)
        with open(os.path.join(i2c_dir, "virt_bus")) as f:
            virt_bus = int(f.read().strip(), 16)
    except (OSError, ValueError) as e:
        _logger.log_warning(f"cannot read i2c channel layout from {i2c_dir}: {e}")
        return []

    pci_dir = os.path.dirname(i2c_dir)
    bdf = os.path.basename(pci_dir)
    return [
        index
        for index in range(num_virt_ch)
        if os.path.exists(f"/sys/bus/pci/devices/{bdf}/i2c-{index + virt_bus}")
    ]


def _fpga_mdio_bus_indices(mdio_dir):
    """Indices of the MDIO buses `mdio_dir` currently has registered.

    A bus is registered as `pci-mdio-<index>` and parented to the PCI device,
    so the live ones can be read back from sysfs instead of assuming every
    declared channel was created.
    """
    try:
        with open(os.path.join(mdio_dir, "num_virt_ch")) as f:
            num_virt_ch = int(f.read().strip(), 16)
    except (OSError, ValueError) as e:
        _logger.log_warning(f"cannot read MDIO bus layout from {mdio_dir}: {e}")
        return []

    bdf = os.path.basename(os.path.dirname(mdio_dir))
    return [
        index
        for index in range(num_virt_ch)
        if os.path.exists(f"/sys/bus/pci/devices/{bdf}/mdio_bus/pci-mdio-{index}")
    ]


def _delete_fpga_mdio_buses():
    """Delete the FPGA MDIO buses so the algorithm module's references drop.

    Registering a bus pins the module providing its read/write ops, released
    only when the bus goes away, so `pddf_custom_mdio_algo` cannot be unloaded
    while any remain, and nothing else in the teardown removes them: they are
    only unregistered when `pddf_multifpgapci_mdio_module` detaches, which the
    unload order reaches after the algorithm module.
    """
    try:
        entries = sorted(os.listdir(MULTIFPGAPCI_DEVICES_PATH))
    except OSError:
        return

    for bdf in entries:
        mdio_dir = os.path.join(MULTIFPGAPCI_DEVICES_PATH, bdf, "mdio")
        if not os.path.isdir(mdio_dir):
            continue
        del_path = os.path.join(mdio_dir, "del_mdio_bus")
        for index in _fpga_mdio_bus_indices(mdio_dir):
            try:
                with open(del_path, "w") as f:
                    f.write(str(index))
            except OSError as e:
                _logger.log_warning(
                    f"could not delete {bdf} MDIO bus {index}, the unload "
                    f"after this will report what is still pinned: {e}"
                )


def _delete_fpga_i2c_adapters():
    """Delete the FPGA i2c adapters so the algorithm module's references drop.

    Registering an adapter takes a reference on the module owning its algorithm,
    released only when the adapter goes away, so `pddf_custom_fpga_algo` cannot
    be unloaded while any remain and nothing else in the teardown removes them.

    Must run only once the PDDF clients on those adapters are gone:
    `i2c_del_adapter()` waits for the references its clients hold, so deleting
    an adapter that still has any blocks forever.
    """
    try:
        entries = sorted(os.listdir(MULTIFPGAPCI_DEVICES_PATH))
    except OSError:
        # No multifpgapci devices on this platform, so no adapters to delete.
        return

    for bdf in entries:
        i2c_dir = os.path.join(MULTIFPGAPCI_DEVICES_PATH, bdf, "i2c")
        # The directory also holds plain attribute files alongside the devices.
        if not os.path.isdir(i2c_dir):
            continue
        del_path = os.path.join(i2c_dir, "del_i2c_adapter")
        for index in _fpga_i2c_adapter_indices(i2c_dir):
            try:
                with open(del_path, "w") as f:
                    f.write(str(index))
            except OSError as e:
                _logger.log_warning(
                    f"could not delete {bdf} i2c adapter {index}, the unload "
                    f"after this will report what is still pinned: {e}"
                )


def _read_console_loglevel():
    with open(PRINTK_PATH, "r") as f:
        return f.read().split()[0]


def _write_console_loglevel(level):
    with open(PRINTK_PATH, "w") as f:
        f.write(str(level))


@contextmanager
def _quieted_console():
    """Keep kernel messages off the console; restore the previous level on exit.

    The PDDF drivers log an unratelimited KERN_ERR per failed FPGA access and the
    teardown makes every access fail. On a 9600-baud console each message costs
    ~100 ms of in-kernel time, and the CPU that ends up flushing stops answering
    IPIs; a broadcast-IPI waiter then hangs until the soft-lockup watchdog panics
    the switch. Losing console quieting is not a reason to refuse a reload, so
    failure to change the level is logged, not raised.
    """
    previous = None
    try:
        previous = _read_console_loglevel()
        _write_console_loglevel(QUIET_CONSOLE_LOGLEVEL)
    except (OSError, IndexError) as e:
        previous = None
        _logger.log_warning(
            f"could not quiet the console for the reload, continuing anyway: {e}"
        )
    try:
        yield
    finally:
        if previous is not None:
            try:
                _write_console_loglevel(previous)
            except OSError as e:
                _logger.log_error(
                    f"could not restore console loglevel to {previous}; kernel "
                    f"messages will not reach the console until it is set again: {e}"
                )


def reload_all_modules(watchdog_seconds=RELOAD_WATCHDOG_SECONDS):
    """Reload every PDDF/custom kernel module and recreate PDDF devices.

    `pddf_util.py clean` deletes the device nodes and then unloads the declared
    modules in a fixed order, aborting on the first one that still has a loaded
    dependent (e.g. nh_pmbus_core under nh_adm1266) — hence `--force`, so that
    abort does not skip the rest of the teardown, and hence the leftovers are
    unloaded here in dependency order.

    Device nodes go before modules: a bound i2c client holds a reference on its
    driver's module, so unloading with the devices live can fail with EBUSY.
    `pddf_util.py install` also silently skips loading drivers while any PDDF
    module is resident, so the teardown is verified complete before handing back.

    The FPGA watchdog stays armed for `watchdog_seconds` with punching paused,
    so a reload stuck past that resets the switch. Raise it when root-causing a
    wedged teardown on a switch that should stay up.
    """
    pddf_kos, custom_kos = get_declared_modules()
    declared = pddf_kos + custom_kos

    with _reload_watchdog(watchdog_seconds), _quieted_console():
        _run_cmd_or_raise(f"{PDDF_UTIL_PATH} --force clean", "pddf_util.py clean failed")

        # `--force clean` reports success even when delete_pddf_devices() failed,
        # so check the devices are really gone rather than trusting the exit code.
        # Unloading with clients still bound is the use-after-free this avoids.
        still_bound = _i2c_clients_bound_to(declared)
        if still_bound:
            raise ModuleReloadError(
                "PDDF devices are still bound after `pddf_util.py --force clean`, "
                "unloading now would leave their drivers pointing at freed module "
                "memory: "
                + ", ".join(
                    f"{mod} ({len(clients)} clients)"
                    for mod, clients in sorted(still_bound.items())
                )
            )

        # Safe only here: the clients are confirmed gone above, and the unload
        # below cannot succeed while the adapters still pin their algo module.
        _delete_fpga_i2c_adapters()
        _delete_fpga_mdio_buses()

        for mod in _unload_order_for(declared, get_loaded_modules()):
            # `modprobe -r` also drops dependencies it leaves unused, so an entry in
            # the order may already be gone by the time it is reached.
            if is_module_loaded(mod):
                _run_cmd_or_raise(f"modprobe -r {mod}", f"Failed to unload {mod}")

        loaded = get_loaded_modules()
        still_loaded = [mod for mod in declared if is_module_loaded(mod, loaded)]
        if still_loaded:
            raise ModuleReloadError(
                "Declared modules still loaded after teardown, `pddf_util.py install` "
                f"would skip loading drivers: {', '.join(still_loaded)}"
            )

        _run_cmd_or_raise(f"{PDDF_UTIL_PATH} install", "pddf_util.py install failed")

        # `install` can report success and still leave the platform half-initialized;
        # fail here rather than hand back a switch whose PDDF stack is incomplete.
        loaded = get_loaded_modules()
        missing = [mod for mod in declared if not is_module_loaded(mod, loaded)]
        if missing:
            raise ModuleReloadError(
                "Declared modules missing after `pddf_util.py install`: "
                f"{', '.join(missing)}"
            )


def reload_module(module_name, watchdog_seconds=RELOAD_WATCHDOG_SECONDS):
    """Unload module_name (and dependents) and load them all back.

    Runs `depmod -a` between unload and reload so a swapped-in .ko file (or
    a rebuilt module) is picked up.

    module_name does not need to already be loaded (e.g. it may have already
    been unloaded to swap its .ko file); in that case this just modprobes it.

    A PDDF-declared module always takes the full teardown/reinstall cycle, and
    that is not overridable. A targeted `modprobe -r` leaves the PDDF device
    nodes in place, and their i2c clients keep driver callbacks pointing into the
    module being removed, so the next poll dereferences freed module text and
    panics the kernel. Recreating the devices afterwards is no better: it re-runs
    the FPGA's `fpgapci_init` while `pddf_multifpgapci_driver` is still live,
    wedging the FPGA past what any later `pddf_util.py install` can recover.
    """
    _validate_module_name(module_name)
    name = _normalize(module_name)

    if is_pddf_module(name):
        # depmod first so a swapped-in .ko is picked up by the reload below.
        _run_cmd_or_raise("depmod -a", "depmod failed")
        reload_all_modules(watchdog_seconds=watchdog_seconds)
        return

    loaded_modules = get_loaded_modules()
    if name in loaded_modules:
        unload_order = get_unload_order(name, loaded_modules)
        reload_order = list(reversed(unload_order))
    else:
        unload_order = []
        reload_order = [name]

    for mod in unload_order:
        _run_cmd_or_raise(f"modprobe -r {mod}", f"Failed to unload {mod}")

    _run_cmd_or_raise("depmod -a", "depmod failed")

    for mod in reload_order:
        _run_cmd_or_raise(f"modprobe {mod}", f"Failed to load {mod}")
